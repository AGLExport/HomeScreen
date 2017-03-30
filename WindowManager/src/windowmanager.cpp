/*
 * Copyright (C) 2016, 2017 Mentor Graphics Development (Deutschland) GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "windowmanager.hpp"


//////////////////////////////////////////
// THIS IS STILL UNDER HEAVY DEVELOPMENT!
// DO NOT JUDGE THE SOURCE CODE :)
//////////////////////////////////////////

// three layers will be defined. The HomeScreen will be placed
// full screen in the background.
// On top all applications in one layer.
// On top of that, the popup layer.
#define WINDOWMANAGER_LAYER_POPUP 100
#define WINDOWMANAGER_LAYER_HOMESCREEN_OVERLAY 101
#define WINDOWMANAGER_LAYER_APPLICATIONS 102
#define WINDOWMANAGER_LAYER_HOMESCREEN 103

#define WINDOWMANAGER_LAYER_NUM 4

#define WINDOWMANAGER_SURFACE_ID_SHIFT 22

// the HomeScreen app has to have the surface id 4194304
#define WINDOWMANAGER_HOMESCREEN_MAIN_SURFACE_ID (1 << WINDOWMANAGER_SURFACE_ID_SHIFT)

// Quick hack for scaling layer to fit non-FHD(1920x1080) screen
//  * source rect of layer should be 1920x1080
//  * destination rect of layer should fit physical display resolution
//  * source rect of surface shoud be based on 1920x1080
//  * destination rect of surface should be based on 1920x1080
#define WINDOWMANAGER_HOMESCREEN_WIDTH  1080
#define WINDOWMANAGER_HOMESCREEN_HEIGHT 1920

void* WindowManager::myThis = 0;

WindowManager::WindowManager(int displayId, QObject *parent) :
    QObject(parent),
    m_layouts(),
    mp_layoutAreaToSurfaceIdAssignment(0),
    m_currentLayout(-1),
    m_screenId(displayId),
    m_screenWidth(0),
    m_screenHeight(0)
#ifdef HAVE_IVI_LAYERMANAGEMENT_API
    ,
    m_appSurfaces(),
    m_appLayers(),
    m_pending_to_show(-1)
#endif
{
#ifdef HAVE_IVI_LAYERMANAGEMENT_API
    m_showLayers = new t_ilm_layer[WINDOWMANAGER_LAYER_NUM];

    m_showLayers[0] = 0; /* POPUP is not shown by default */
    m_showLayers[1] = 0; /* HOMESCREEN_OVERLAY is not shown by default */
    m_showLayers[2] = 0; /* APPLICATIONS is not shown by default */
    m_showLayers[3] = WINDOWMANAGER_LAYER_HOMESCREEN; /* HOMESCREEN is shwon by default */

#endif
    qDebug("-=[WindowManager]=-");
}

void WindowManager::start()
{
    qDebug("-=[start]=-");
    mp_layoutAreaToSurfaceIdAssignment = new QMap<int, unsigned int>;
#ifdef HAVE_IVI_LAYERMANAGEMENT_API
    ilmErrorTypes err;

    err = ilm_init();
    qDebug("ilm_init = %d", err);
    if(ILM_SUCCESS != err)
    {
        qDebug("failed! Exiting!");
        exit(-1);
    }

    myThis = this;

    ilm_getScreenResolution(m_screenId, &m_screenWidth, &m_screenHeight);

    createNewLayer(WINDOWMANAGER_LAYER_POPUP);
    createNewLayer(WINDOWMANAGER_LAYER_HOMESCREEN_OVERLAY);
    createNewLayer(WINDOWMANAGER_LAYER_HOMESCREEN);

    ilm_registerNotification(WindowManager::notificationFunc_static, this);
#endif

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject("/windowmanager", this);
    dbus.registerService("org.agl.windowmanager");

    // publish windowmanager interface
    mp_windowManagerAdaptor = new WindowmanagerAdaptor((QObject*)this);
}

WindowManager::~WindowManager()
{
    qDebug("-=[~WindowManager]=-");
    delete mp_windowManagerAdaptor;
#ifdef HAVE_IVI_LAYERMANAGEMENT_API
    ilm_destroy();
#endif
    delete mp_layoutAreaToSurfaceIdAssignment;
}

#ifdef HAVE_IVI_LAYERMANAGEMENT_API
int WindowManager::getLayerRenderOrder(t_ilm_layer id_array[])
{
    int i, j;

    for (i = WINDOWMANAGER_LAYER_NUM - 1, j = 0; i >= 0; i--) {
        if (m_showLayers[i] != 0) {
            id_array[j++] = m_showLayers[i];
        }
    }

    return j;
}
#endif

void WindowManager::dumpScene()
{
    qDebug("\n");
    qDebug("current layout   : %d", m_currentLayout);
    qDebug("available layouts: %d", m_layouts.size());
    QList<Layout>::const_iterator i = m_layouts.begin();

    while (i != m_layouts.constEnd())
    {
        qDebug("--[id: %d]--[%s]--", i->id, i->name.toStdString().c_str());
        qDebug("  %d surface areas", i->layoutAreas.size());
        for (int j = 0; j < i->layoutAreas.size(); ++j)
        {
            qDebug("  -area %d", j);
            qDebug("    -x     : %d", i->layoutAreas.at(j).x);
            qDebug("    -y     : %d", i->layoutAreas.at(j).y);
            qDebug("    -width : %d", i->layoutAreas.at(j).width);
            qDebug("    -height: %d", i->layoutAreas.at(j).height);
        }

        ++i;
    }
}

#ifdef HAVE_IVI_LAYERMANAGEMENT_API

void WindowManager::createNewLayer(int layerId)
{
    qDebug("-=[createNewLayer]=-");
    qDebug("  layerId %d", layerId);

    t_ilm_layer newLayerId = layerId;
    ilm_layerCreateWithDimension(&newLayerId,
                                    WINDOWMANAGER_HOMESCREEN_WIDTH,
                                    WINDOWMANAGER_HOMESCREEN_HEIGHT);
    ilm_commitChanges();
    ilm_layerSetSourceRectangle(newLayerId,
                                    0,
                                    0,
                                    WINDOWMANAGER_HOMESCREEN_WIDTH,
                                    WINDOWMANAGER_HOMESCREEN_HEIGHT);
    ilm_layerSetDestinationRectangle(newLayerId,
                                    0,
                                    0,
                                    m_screenWidth,
                                    m_screenHeight);
    ilm_commitChanges();
    ilm_layerSetOpacity(newLayerId, 1.0);
    ilm_layerSetVisibility(newLayerId, ILM_TRUE);
    ilm_commitChanges();
}

t_ilm_layer WindowManager::getAppLayerID(pid_t pid)
{
    t_ilm_layer layer_id;

//    layer_id = pid + (WINDOWMANAGER_LAYER_APPLICATIONS << WINDOWMANAGER_LAYER_ID_SHIFT);
    layer_id = pid + (WINDOWMANAGER_LAYER_APPLICATIONS * 100000); /* for debug */

    return layer_id;
}

void WindowManager::addSurface(t_ilm_surface surfaceId)
{
    struct ilmSurfaceProperties surfaceProperties;
    pid_t pid;

    ilm_getPropertiesOfSurface(surfaceId, &surfaceProperties);
    pid = surfaceProperties.creatorPid;

    QMap<pid_t, t_ilm_surface>::const_iterator i = m_appSurfaces.find(pid);
    if (i != m_appSurfaces.end() && i.value() == 0) {
        /* Only the 1st surface is handled by Window Manager */
        qDebug("This surface (%d) is 1st one for app (%d)", surfaceId, pid);
        /* update surface id */
        m_appSurfaces.insert(pid, surfaceId);

        /* this surface should be handled by WindowManager */
        ilm_surfaceAddNotification(surfaceId, surfaceCallbackFunction_static);
        ilm_commitChanges();
    }
}

t_ilm_layer WindowManager::addSurfaceToAppLayer(pid_t pid, int surfaceId)
{
    struct ilmSurfaceProperties surfaceProperties;
    t_ilm_layer layer_id;
    int found = 0;

    qDebug("-=[addSurfaceToAppLayer]=-");
    qDebug("  surfaceId %d", surfaceId);

    if (pid < 0)
        return 0;

    QMap<pid_t, t_ilm_layer>::const_iterator i = m_appLayers.find(pid);
    if (i == m_appLayers.end()) {
        qDebug("No layer found, create new for app(pid=%d)", pid);

        /* not found, create new one */
        layer_id = getAppLayerID(pid);

        createNewLayer(layer_id);
        m_appLayers.insert(pid, layer_id);
    } else {
        layer_id = i.value();
    }

    return layer_id;
}

void WindowManager::addSurfaceToLayer(int surfaceId, int layerId)
{
    qDebug("-=[addSurfaceToLayer]=-");
    qDebug("  surfaceId %d", surfaceId);
    qDebug("  layerId %d", layerId);

    if (layerId == WINDOWMANAGER_LAYER_HOMESCREEN)
    {
      ilm_layerAddSurface(layerId, surfaceId);
    }
    else if (layerId == WINDOWMANAGER_LAYER_HOMESCREEN_OVERLAY)
    {
        struct ilmSurfaceProperties surfaceProperties;
        ilm_getPropertiesOfSurface(surfaceId, &surfaceProperties);

        //ilm_surfaceSetDestinationRectangle(surfaceId, 0, 0, surfaceProperties.origSourceWidth, surfaceProperties.origSourceHeight);
        //ilm_surfaceSetSourceRectangle(surfaceId, 0, 0, surfaceProperties.origSourceWidth, surfaceProperties.origSourceHeight);
        //ilm_surfaceSetOpacity(surfaceId, 0.5);
        //ilm_surfaceSetVisibility(surfaceId, ILM_TRUE);

        ilm_layerAddSurface(layerId, surfaceId);
    }
    else if (layerId == WINDOWMANAGER_LAYER_POPUP)
    {
        struct ilmSurfaceProperties surfaceProperties;
        ilm_getPropertiesOfSurface(surfaceId, &surfaceProperties);

        //ilm_surfaceSetDestinationRectangle(surfaceId, 0, 0, surfaceProperties.origSourceWidth, surfaceProperties.origSourceHeight);
        //ilm_surfaceSetSourceRectangle(surfaceId, 0, 0, surfaceProperties.origSourceWidth, surfaceProperties.origSourceHeight);
        //ilm_surfaceSetOpacity(surfaceId, 0.0);
        //ilm_surfaceSetVisibility(surfaceId, ILM_FALSE);

        ilm_layerAddSurface(layerId, surfaceId);
    } else {
        return;
    }

    ilm_commitChanges();
}

void WindowManager::configureHomeScreenMainSurface(t_ilm_surface surface, t_ilm_int width, t_ilm_int height)
{
    // homescreen app always fullscreen in the back
    ilm_surfaceSetDestinationRectangle(surface, 0, 0,
                                       WINDOWMANAGER_HOMESCREEN_WIDTH,
                                       WINDOWMANAGER_HOMESCREEN_HEIGHT);
    ilm_surfaceSetSourceRectangle(surface, 0, 0, width, height);
    ilm_surfaceSetOpacity(surface, 1.0);
    ilm_surfaceSetVisibility(surface, ILM_TRUE);

    ilm_commitChanges();
}

void WindowManager::configureAppSurface(pid_t pid, t_ilm_surface surface, t_ilm_int width, t_ilm_int height)
{
    /* Dirty hack! cut & paste from HomeScreen/src/layouthandler.cpp */
    const int SCREEN_WIDTH = 1080;
    const int SCREEN_HEIGHT = 1920;

    const int TOPAREA_HEIGHT = 218;
    const int TOPAREA_WIDTH = SCREEN_WIDTH;
    const int TOPAREA_X = 0;
    const int TOPAREA_Y = 0;
    const int MEDIAAREA_HEIGHT = 215;
    const int MEDIAAREA_WIDTH = SCREEN_WIDTH;
    const int MEDIAAREA_X = 0;
    const int MEDIAAREA_Y = SCREEN_HEIGHT - MEDIAAREA_HEIGHT;

	const int APP_WIDTH  = SCREEN_WIDTH;
	const int APP_HEIGHT = (SCREEN_HEIGHT - TOPAREA_HEIGHT - MEDIAAREA_HEIGHT);
	int dest_left,dest_top,dest_width,dest_height;

#if 1
		dest_width = width;
		dest_height = height;
		dest_left = (APP_WIDTH - dest_width) / 2;
		dest_top = (APP_HEIGHT - dest_height) / 2 + TOPAREA_HEIGHT;
#else
	double sx,sy;
	
	//スケーリング係数を計算
	sx = double(APP_WIDTH) / double(width);
	sy = double(APP_HEIGHT) / double(height);
	
	if (sx > sy)
	{
		//横の拡大率が大きいので、縦を規準にサイズを調整
		dest_width = width * APP_HEIGHT / height;
		dest_height = height * APP_HEIGHT / height;
		dest_left = (APP_WIDTH - dest_width) / 2;
		dest_top = (APP_HEIGHT - dest_height) / 2 + TOPAREA_HEIGHT;
	}
	else
	{
		//縦の拡大率が大きいので、横のサイズを規準に調整	
		dest_width = width * APP_WIDTH / width;
		dest_height = height * APP_WIDTH / width;
		dest_left = (APP_WIDTH - dest_width) / 2;
		dest_top = (APP_HEIGHT - dest_height) / 2 + TOPAREA_HEIGHT;
	}
#endif
    ilm_surfaceSetDestinationRectangle(surface,
                                       dest_left,
                                       dest_top,
                                       dest_width,
                                       dest_height);
//    ilm_surfaceSetDestinationRectangle(surface,
//                                       0,
//                                       TOPAREA_HEIGHT,
//                                       SCREEN_WIDTH,
//                                       SCREEN_HEIGHT - TOPAREA_HEIGHT - MEDIAAREA_HEIGHT);
    ilm_surfaceSetSourceRectangle(surface, 0, 0, width, height);
    ilm_surfaceSetOpacity(surface, 1.0);
    ilm_surfaceSetVisibility(surface, ILM_TRUE); /* Hack to avoid blank screen when switch apps */
    
    ilm_commitChanges();
}
#endif

void WindowManager::updateScreen()
{
    qDebug("-=[updateScreen]=-");

#if 0
//#ifdef HAVE_IVI_LAYERMANAGEMENT_API
    if (-1 != m_currentLayout)
    {
        // hide all surfaces
        for (int i = 0; i < m_appSurfaces.size(); ++i)
        {
            ilm_layerRemoveSurface(WINDOWMANAGER_LAYER_APPLICATIONS, m_appSurfaces.at(i));
            //ilm_surfaceSetVisibility(m_appSurfaces.at(i), ILM_FALSE);
            //ilm_surfaceSetOpacity(m_appSurfaces.at(i), 0.0);
            ilm_commitChanges();
        }

        // find the current used layout
        QList<Layout>::const_iterator ci = m_layouts.begin();

        Layout currentLayout;
        while (ci != m_layouts.constEnd())
        {
            if (ci->id == m_currentLayout)
            {
                currentLayout = *ci;
            }

            ++ci;
        }

        qDebug("show %d apps", mp_layoutAreaToSurfaceIdAssignment->size());
        for (int j = 0; j < mp_layoutAreaToSurfaceIdAssignment->size(); ++j)
        {
            int surfaceToShow = mp_layoutAreaToSurfaceIdAssignment->find(j).value();
            qDebug("  surface no. %d: %d", j, surfaceToShow);

            addSurfaceToLayer(surfaceToShow, WINDOWMANAGER_LAYER_APPLICATIONS);

            ilm_surfaceSetVisibility(surfaceToShow, ILM_TRUE);
            ilm_surfaceSetOpacity(surfaceToShow, 1.0);

            qDebug("  layout area %d", j);
            qDebug("    x: %d", currentLayout.layoutAreas[j].x);
            qDebug("    y: %d", currentLayout.layoutAreas[j].y);
            qDebug("    w: %d", currentLayout.layoutAreas[j].width);
            qDebug("    h: %d", currentLayout.layoutAreas[j].height);

            ilm_surfaceSetDestinationRectangle(surfaceToShow,
                                             currentLayout.layoutAreas[j].x,
                                             currentLayout.layoutAreas[j].y,
                                             currentLayout.layoutAreas[j].width,
                                             currentLayout.layoutAreas[j].height);
            ilm_commitChanges();
        }
    }

    // layer surface render order
    t_ilm_int length;
    t_ilm_surface* pArray;
    ilm_getSurfaceIDsOnLayer(WINDOWMANAGER_LAYER_HOMESCREEN, &length, &pArray);
    ilm_layerSetRenderOrder(WINDOWMANAGER_LAYER_HOMESCREEN, pArray, length);
    ilm_commitChanges();
    ilm_getSurfaceIDsOnLayer(WINDOWMANAGER_LAYER_APPLICATIONS, &length, &pArray);
    ilm_layerSetRenderOrder(WINDOWMANAGER_LAYER_APPLICATIONS, pArray, length);
    ilm_commitChanges();
    ilm_getSurfaceIDsOnLayer(WINDOWMANAGER_LAYER_HOMESCREEN_OVERLAY, &length, &pArray);
    ilm_layerSetRenderOrder(WINDOWMANAGER_LAYER_HOMESCREEN_OVERLAY, pArray, length);
    ilm_commitChanges();
    ilm_getSurfaceIDsOnLayer(WINDOWMANAGER_LAYER_POPUP, &length, &pArray);
    ilm_layerSetRenderOrder(WINDOWMANAGER_LAYER_POPUP, pArray, length);
    ilm_commitChanges();
#endif
#ifdef HAVE_IVI_LAYERMANAGEMENT_API
    if (m_pending_to_show != -1) {
        qDebug("show pending app (%d)", m_pending_to_show);
        showAppLayer(m_pending_to_show);
    } else {
        // display layer render order
        t_ilm_layer renderOrder[WINDOWMANAGER_LAYER_NUM];
        int num_layers = getLayerRenderOrder(renderOrder);

        qDebug("Screen render order %d, %d layers", m_screenId, num_layers);
        ilm_displaySetRenderOrder(m_screenId, renderOrder, num_layers);
        ilm_commitChanges();
    }
#endif
}

#ifdef HAVE_IVI_LAYERMANAGEMENT_API
void WindowManager::notificationFunc_non_static(ilmObjectType object,
                                    t_ilm_uint id,
                                    t_ilm_bool created)
{
    qDebug("-=[notificationFunc_non_static]=-");
    qDebug("Notification from weston!");
    if (ILM_SURFACE == object)
    {
        struct ilmSurfaceProperties surfaceProperties;

        if (created)
        {
            if (WINDOWMANAGER_HOMESCREEN_MAIN_SURFACE_ID == id)
            {
                ilm_surfaceAddNotification(id, surfaceCallbackFunction_static);
                ilm_commitChanges();
            }
            else
            {
                addSurface(id);
            }
        }
        else
        {
            qDebug("Surface destroyed, ID: %d", id);
#if 0
            m_appSurfaces.removeAt(m_appSurfaces.indexOf(id));
            ilm_surfaceRemoveNotification(id);

            ilm_commitChanges();
#endif
        }
    }
    if (ILM_LAYER == object)
    {
        //qDebug("Layer.. we don't care...");
    }
}

void WindowManager::notificationFunc_static(ilmObjectType object,
                                            t_ilm_uint id,
                                            t_ilm_bool created,
                                            void* user_data)
{
    static_cast<WindowManager*>(WindowManager::myThis)->notificationFunc_non_static(object, id, created);
}

void WindowManager::surfaceCallbackFunction_non_static(t_ilm_surface surface,
                                    struct ilmSurfaceProperties* surfaceProperties,
                                    t_ilm_notification_mask mask)
{
    qDebug("-=[surfaceCallbackFunction_non_static]=-");
    qDebug("surfaceCallbackFunction_non_static changes for surface %d", surface);
    if (ILM_NOTIFICATION_VISIBILITY & mask)
    {
        qDebug("ILM_NOTIFICATION_VISIBILITY");
        surfaceVisibilityChanged(surface, surfaceProperties->visibility);

		//入力デバイスのフォーカスを設定
		//ilm_setInputFocus(&surface, 1, ILM_INPUT_DEVICE_ALL, ILM_TRUE);
		
        updateScreen();
    }
    if (ILM_NOTIFICATION_OPACITY & mask)
    {
        qDebug("ILM_NOTIFICATION_OPACITY");
    }
    if (ILM_NOTIFICATION_ORIENTATION & mask)
    {
        qDebug("ILM_NOTIFICATION_ORIENTATION");
    }
    if (ILM_NOTIFICATION_SOURCE_RECT & mask)
    {
        qDebug("ILM_NOTIFICATION_SOURCE_RECT");
    }
    if (ILM_NOTIFICATION_DEST_RECT & mask)
    {
        qDebug("ILM_NOTIFICATION_DEST_RECT");
    }
    if (ILM_NOTIFICATION_CONTENT_AVAILABLE & mask)
    {
        qDebug("ILM_NOTIFICATION_CONTENT_AVAILABLE");
    }
    if (ILM_NOTIFICATION_CONTENT_REMOVED & mask)
    {
        qDebug("ILM_NOTIFICATION_CONTENT_REMOVED");

        /* application being down */
        m_appLayers.remove(surfaceProperties->creatorPid);
    }
    if (ILM_NOTIFICATION_CONFIGURED & mask)
    {
        qDebug("ILM_NOTIFICATION_CONFIGURED");
        qDebug("  surfaceProperties %d", surface);
        qDebug("    surfaceProperties.origSourceWidth: %d", surfaceProperties->origSourceWidth);
        qDebug("    surfaceProperties.origSourceHeight: %d", surfaceProperties->origSourceHeight);

        if (surface == WINDOWMANAGER_HOMESCREEN_MAIN_SURFACE_ID) {
            addSurfaceToLayer(surface, WINDOWMANAGER_LAYER_HOMESCREEN);
            configureHomeScreenMainSurface(surface, surfaceProperties->origSourceWidth, surfaceProperties->origSourceHeight);
        } else {
            ilmErrorTypes result;
            pid_t pid = surfaceProperties->creatorPid;
            t_ilm_layer layer = addSurfaceToAppLayer(pid, surface);

            if (layer != 0) {
                configureAppSurface(pid, surface,
                                    surfaceProperties->origSourceWidth,
                                    surfaceProperties->origSourceHeight);

                result = ilm_layerAddSurface(layer, surface);
                if (result != ILM_SUCCESS) {
                    qDebug("ilm_layerAddSurface(%d,%d) failed.", layer, surface);
                }
                
                
				//入力デバイスのフォーカスを設定
				//ilm_setInputFocus(&surface, 1, ILM_INPUT_DEVICE_ALL, ILM_TRUE);
                
                ilm_commitChanges();
            }
        }
        updateScreen();
    }
}

void WindowManager::surfaceCallbackFunction_static(t_ilm_surface surface,
                                    struct ilmSurfaceProperties* surfaceProperties,
                                    t_ilm_notification_mask mask)

{
    static_cast<WindowManager*>(WindowManager::myThis)->surfaceCallbackFunction_non_static(surface, surfaceProperties, mask);
}
#endif

int WindowManager::layoutId() const
{
    return m_currentLayout;
}

QString WindowManager::layoutName() const
{
    QList<Layout>::const_iterator i = m_layouts.begin();

    QString result = "not found";
    while (i != m_layouts.constEnd())
    {
        if (i->id == m_currentLayout)
        {
            result = i->name;
        }

        ++i;
    }

    return result;
}


int WindowManager::addLayout(int layoutId, const QString &layoutName, const QList<LayoutArea> &surfaceAreas)
{
    qDebug("-=[addLayout]=-");
    m_layouts.append(Layout(layoutId, layoutName, surfaceAreas));

    qDebug("addLayout %d %s, size %d",
           layoutId,
           layoutName.toStdString().c_str(),
           surfaceAreas.size());

    dumpScene();

    return WINDOWMANAGER_NO_ERROR;
}

int WindowManager::deleteLayoutById(int layoutId)
{
    qDebug("-=[deleteLayoutById]=-");
    qDebug("layoutId: %d", layoutId);
    int result = WINDOWMANAGER_NO_ERROR;

    if (m_currentLayout == layoutId)
    {
        result = WINDOWMANAGER_ERROR_ID_IN_USE;
    }
    else
    {
        QList<Layout>::iterator i = m_layouts.begin();
        result = WINDOWMANAGER_ERROR_ID_IN_USE;
        while (i != m_layouts.constEnd())
        {
            if (i->id == layoutId)
            {
                m_layouts.erase(i);
                result = WINDOWMANAGER_NO_ERROR;
                break;
            }

            ++i;
        }
    }

    return result;
}


QList<Layout> WindowManager::getAllLayouts()
{
    qDebug("-=[getAllLayouts]=-");

    return m_layouts;
}

#if 0
QList<int> WindowManager::getAllSurfacesOfProcess(int pid)
{
    QList<int> result;
#ifdef HAVE_IVI_LAYERMANAGEMENT_API
    struct ilmSurfaceProperties surfaceProperties;

    for (int i = 0; i < m_appSurfaces.size(); ++i)
    {
        ilm_getPropertiesOfSurface(m_appSurfaces.at(i), &surfaceProperties);
        if (pid == surfaceProperties.creatorPid)
        {
            result.append(m_appSurfaces.at(i));
        }
    }
#endif
    return result;
}
#endif

QList<int> WindowManager::getAvailableLayouts(int numberOfAppSurfaces)
{
    qDebug("-=[getAvailableLayouts]=-");
    QList<Layout>::const_iterator i = m_layouts.begin();

    QList<int> result;
    while (i != m_layouts.constEnd())
    {
        if (i->layoutAreas.size() == numberOfAppSurfaces)
        {
            result.append(i->id);
        }

        ++i;
    }

    return result;
}

#if 0
QList<int> WindowManager::getAvailableSurfaces()
{
    qDebug("-=[getAvailableSurfaces]=-");

    return m_appSurfaces;
}
#endif

QString WindowManager::getLayoutName(int layoutId)
{
    qDebug("-=[getLayoutName]=-");
    QList<Layout>::const_iterator i = m_layouts.begin();

    QString result = "not found";
    while (i != m_layouts.constEnd())
    {
        if (i->id == layoutId)
        {
            result = i->name;
        }

        ++i;
    }

    return result;
}

void WindowManager::hideLayer(int layer)
{
    qDebug("-=[hideLayer]=-");
    qDebug("layer %d", layer);

#ifdef HAVE_IVI_LAYERMANAGEMENT_API
    // POPUP=0, HOMESCREEN_OVERLAY=1, APPS=2, HOMESCREEN=3
    if (layer >= 0 && layer < WINDOWMANAGER_LAYER_NUM) {
        /* hide target layer */
        m_showLayers[layer] = 0;
        if (layer == 2) {
            /* clear pending flag */
            m_pending_to_show = -1;
        }

        t_ilm_layer renderOrder[WINDOWMANAGER_LAYER_NUM];
        int num_layers = getLayerRenderOrder(renderOrder);
        ilm_displaySetRenderOrder(m_screenId, renderOrder, num_layers);
        ilm_commitChanges();
    }
#endif
}

int WindowManager::setLayoutById(int layoutId)
{
    qDebug("-=[setLayoutById]=-");
    int result = WINDOWMANAGER_NO_ERROR;
    m_currentLayout = layoutId;

    mp_layoutAreaToSurfaceIdAssignment->clear();

    dumpScene();

    return result;
}

int WindowManager::setLayoutByName(const QString &layoutName)
{
    qDebug("-=[setLayoutByName]=-");
    int result = WINDOWMANAGER_NO_ERROR;

    QList<Layout>::const_iterator i = m_layouts.begin();

    while (i != m_layouts.constEnd())
    {
        if (i->name == layoutName)
        {
            m_currentLayout = i->id;

            mp_layoutAreaToSurfaceIdAssignment->clear();

            dumpScene();
        }

        ++i;
    }

    return result;
}

int WindowManager::setSurfaceToLayoutArea(int surfaceId, int layoutAreaId)
{
    qDebug("-=[setSurfaceToLayoutArea]=-");
    int result = WINDOWMANAGER_NO_ERROR;

    qDebug("surfaceId %d", surfaceId);
    qDebug("layoutAreaId %d", layoutAreaId);
    mp_layoutAreaToSurfaceIdAssignment->insert(layoutAreaId, surfaceId);

    updateScreen();

    dumpScene();

    return result;
}

void WindowManager::showLayer(int layer)
{
    qDebug("-=[showLayer]=-");
    qDebug("layer %d", layer);

#ifdef HAVE_IVI_LAYERMANAGEMENT_API
    // POPUP=0, HOMESCREEN_OVERLAY=1, APPS=2, HOMESCREEN=3
    if (layer >= 0 && layer < WINDOWMANAGER_LAYER_NUM) {
        static const int layer_id_array[] = {
            WINDOWMANAGER_LAYER_POPUP,
            WINDOWMANAGER_LAYER_HOMESCREEN_OVERLAY,
            WINDOWMANAGER_LAYER_APPLICATIONS,
            WINDOWMANAGER_LAYER_HOMESCREEN,
        };

        m_showLayers[layer] = layer_id_array[layer];

        t_ilm_layer renderOrder[WINDOWMANAGER_LAYER_NUM];
        int num_layers = getLayerRenderOrder(renderOrder);
        ilm_displaySetRenderOrder(m_screenId, renderOrder, num_layers);
        ilm_commitChanges();
    }
#endif
}

void WindowManager::showAppLayer(int pid)
{
    qDebug("-=[showAppLayer]=-");
    qDebug("pid %d", pid);

    if (pid == -1) {
        /* nothing to show */
        return;
    }
#ifdef HAVE_IVI_LAYERMANAGEMENT_API
    /* clear pending flag */
    m_pending_to_show = -1;

    /* search layer id for application to show */
    QMap<pid_t, t_ilm_layer>::const_iterator i = m_appLayers.find(pid);
    QMap<pid_t, t_ilm_surface>::const_iterator j = m_appSurfaces.find(pid);
    t_ilm_surface surface = j.value();

    if (i != m_appLayers.end()) {
        m_showLayers[2] = i.value();
        
        ilm_setInputFocus(&surface, 1, ILM_INPUT_DEVICE_ALL, ILM_TRUE);
        
        qDebug("Found layer(%d) to show for app(pid=%d)", m_showLayers[2], pid);
    } else {
        /* check if this app is registered */
        if (j == m_appSurfaces.end()) {
            qDebug("New app %d", pid);
            m_appSurfaces.insert(pid, 0); /* register pid only so far */
        }

        /* Probably app layer hasn't been made yet */
        m_pending_to_show = pid;
        /* hide current app once, back to default screen */
        m_showLayers[2] = 0;
        
        ilm_setInputFocus(&surface, 1, ILM_INPUT_DEVICE_ALL, ILM_TRUE);
        
        qDebug("No layer to show for app(pid=%d)", pid);
    }
    t_ilm_layer renderOrder[WINDOWMANAGER_LAYER_NUM];

    int num_layers = getLayerRenderOrder(renderOrder);
    ilm_displaySetRenderOrder(m_screenId, renderOrder, num_layers);
    ilm_commitChanges();
#endif
}
