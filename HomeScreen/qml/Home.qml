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
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import Home 1.0

Item {
    id: root
    property int pid: -1
    signal languageChanged
    signal disconnect

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
        id: flagLanguage
        scale: 0.7
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: parent.top
        anchors.topMargin: 10
        source: './images/us_flag.png'
        visible: true
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
        Label {
            id: cardNumber
            anchors.top: parent.bottom
            anchors.topMargin: 10
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment:  Text.AlignHCenter
            color: "white"
            text: "111"
            font.pixelSize: 20
            font.family: "Roboto"
        }
    }
    Item {
        id: hello
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40
        visible: true
        Text {
            id: helloText
            anchors.centerIn: parent
            color: 'white'
            text: 'No Authenticated User'
            font.pixelSize: 40
            font.family: 'Roboto'
            SequentialAnimation on font.letterSpacing {
                id: animation1
                loops: 1;
                NumberAnimation { from: 0; to: 50; easing.type: Easing.InQuad; duration: 3000 }
                running: false
                onRunningChanged: {
                    if(running) {
                        hello.visible = true
                    } else {
                        helloText.opacity = 1
                        helloText.font.letterSpacing = 0
                    }
                }
            }

            SequentialAnimation on opacity {
                id: animation2
                loops: 1;
                running: false
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

    function showVisa(show, num) {
        visa.visible = show
        cardNumber.text = num;
    }
    function changeFlag(flagImage) {
        flagLanguage.source = flagImage
    }
    function setUser(type, auts) {
        if(type === '') {
            authorisations.visible = false
        } else {
            authorisations.visible = true
            labelUserType.text = type
            myModel.clear()
            for (var i=0; i<auts.length; i++) {
                if(auts[i] !== '')
                    myModel.append({"name": auts[i]})
            }
        }
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
                id: appImage
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
                        appImage.source = './images/HMI_AppLauncher_%1_%2-01.png'.arg(model.icon).arg(pressed ? 'Active' : 'Inactive')
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
    ListModel {
        id: myModel
        ListElement {
            name: 'Install App'
        }
        ListElement {
            name: 'Open Trunk'
        }
        ListElement {
            name: 'Update Software'
        }
        ListElement {
            name: 'View Online'
        }
    }
    Item {
        id: authorisations
        anchors.fill: parent
        visible: false
        GridLayout {
            id: gridAut
            columns: 2
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.bottomMargin: 50
            anchors.leftMargin: 20
            Repeater {
                model: myModel
                Image {
                    source: './images/' + model.name + '.png'
                    width: sourceSize.width
                    height: sourceSize.height
                    visible: true
                }
            }
        }
        Label {
            id: labelUserType
            anchors.bottom: gridAut.top
            anchors.bottomMargin: 10
            anchors.left: gridAut.left
            color: "white"
            text: "Owner"
            font.pixelSize: 30
            font.family: "Roboto"
        }
    }

    Image {
        id: logout
        width: sourceSize.width
        height: sourceSize.height
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.bottomMargin: 10
        anchors.rightMargin: 20
        source: './images/Logout-01.png'
        visible: true
        MouseArea {
            anchors.fill: parent
            onClicked: {
                rotateLogout.start()
                disconnect()
                helloText.text= 'No Authenticated User'

            }
        }
        RotationAnimator {
            id: rotateLogout
            target: logout;
            from: 0;
            to: 360;
            loops: 1
            duration: 500
            running: false
        }
    }
}
