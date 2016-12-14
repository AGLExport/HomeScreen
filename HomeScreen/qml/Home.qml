/*
 * Copyright (C) 2016 The Qt Company Ltd.
 * Copyright (C) 2016 Mentor Graphics Development (Deutschland) GmbH
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

import QtQuick 2.2
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.0
import Home 1.0

Item {
    id: root
    property int pid: -1
    signal languageChanged

    Image {
        anchors.fill: parent
        anchors.topMargin: -218
        anchors.bottomMargin: -215
        source: './images/AGL_HMI_Background_Car-01.png'
    }
    Image {
        id: sign90
        width: 200
        height: 200
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 20
        source: './images/B14-90.png'
        visible: false
    }
    Image {
        id: visa
        width: 200
        height: 124
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.top: parent.top
        anchors.topMargin: 20
        source: './images/visa.png'
        visible: false
    }
    Item {
        id: hello
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40
        visible: false
        Text {
            id: helloText
            anchors.centerIn: parent
            color: "white"
            text: "Hello José!"
            font.pixelSize: 40
            font.family: "Roboto"
            SequentialAnimation on font.letterSpacing {
                id: animation1
                loops: 1;
                NumberAnimation { from: 0; to: 50; easing.type: Easing.InQuad; duration: 3000 }
                onRunningChanged: {
                    if(running)
                        hello.visible = true
                    else
                        hello.visible = false
                }
            }

            SequentialAnimation on opacity {
                id: animation2
                loops: 1;
                NumberAnimation { from: 1; to: 0; duration: 2600 }
                PauseAnimation { duration: 400 }
            }
        }
    }
    function showHello(helloString) {
        helloText.text = helloString
        animation1.running = true;
        animation2.running = true;
    }

    function showSign90(show) {
        sign90.visible = show
    }

    function showVisa(show) {
        visa.visible = show
    }
    GridView {
        anchors.centerIn: parent
        width: cellHeight * 3
        height: cellHeight * 3
        cellWidth: 320
        cellHeight: 320

        model: ApplicationModel {}
        delegate: MouseArea {
            width: 320
            height: 320
            Image {
                anchors.fill: parent
                source: './images/HMI_AppLauncher_%1_%2-01.png'.arg(model.icon).arg(pressed ? 'Active' : 'Inactive')
                Label {
                    id: labelName
                    anchors.horizontalCenter: parent.horizontalCenter
                    horizontalAlignment: Text.AlignHCenter
                    y: 257
                    font.pixelSize: 32
                    font.family: "Roboto"
                    color: "white"
                    text: '%1'.arg(model.name)
                    function myChangeLanguage() {
                        text = '%1'.arg(model.name)
                    }
                    Component.onCompleted: {
                        root.languageChanged.connect(myChangeLanguage)
                    }
                }
            }
            onClicked: {
                console.log("app is ", model.id)
                pid = launcher.launch(model.id)
                if (1 < pid) {
                    layoutHandler.makeMeVisible(pid)

                    applicationArea.visible = true
                    appLauncherAreaLauncher.visible = false
                    layoutHandler.showAppLayer(pid)
                }
                else {
                    console.warn("app cannot be launched!")
                }
            }
        }
    }
}
