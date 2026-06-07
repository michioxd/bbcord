import bb.cascades 1.4

Page {
    id: mainPage

    property variant navigationPane: null
    property variant activeContent: null

    Container {
            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }
            
            Container {
                preferredWidth: ui.du(12)
                verticalAlignment: VerticalAlignment.Fill
                background: Color.create("#111111")
                
                ListView {
                    dataModel: serverModel

                    onTriggered: {
                        var item = serverModel.data(indexPath)

                        if (item.type == "dm") {
                            mainPage.loadDmList()
                        } else if (item.type == "server") {
                            mainPage.loadServerList(item.name)
                        }
                    }
                    
                    listItemComponents: [
                        ListItemComponent {
                            type: ""

                            Container {
                                preferredWidth: ui.du(10)
                                preferredHeight: ui.du(10)

                                topMargin: ui.du(1)
                                bottomMargin: ui.du(1)

                                background: Color.create("#333333")

                                layout: DockLayout {}

                                ImageView {
                                    imageSource: ListItemData.icon
                                    visible: ListItemData.icon !== ""
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    verticalAlignment: VerticalAlignment.Fill
                                    scalingMethod: ScalingMethod.AspectFit
                                }

                                Label {
                                    text: ListItemData.name

                                    visible: ListItemData.icon === ""

                                    horizontalAlignment: HorizontalAlignment.Center
                                    verticalAlignment: VerticalAlignment.Center

                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.fontSize: FontSize.Large
                                }
                            }
                        }
                    ]
                    leftPadding: ui.du(1.0)
                    topPadding: ui.du(1.0)
                }
            }

            Container {
                id: contentHost

                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill

                layout: StackLayout {}
            }
        }

        function openChat(channelName) {
            var page = chatCardDefinition.createObject()

            if (page) {
                page.channelName = channelName
                page.title = "# " + channelName
                page.backRequested.connect(function() {
                        if (mainPage.navigationPane) {
                            mainPage.navigationPane.pop()
                        }
                    })
                page.memberListRequested.connect(function() {
                        var memberPage = channelMemberListDefinition.createObject()

                        if (memberPage) {
                            memberPage.channelName = channelName
                            memberPage.title = "Members #" + channelName
                            memberPage.backRequested.connect(function() {
                                    if (mainPage.navigationPane) {
                                        mainPage.navigationPane.pop()
                                    }
                                })
                            if (mainPage.navigationPane) {
                                mainPage.navigationPane.push(memberPage)
                            }
                        } else {
                            console.log("Could not create ChannelMemberList.qml")
                        }
                    })
                if (mainPage.navigationPane) {
                    mainPage.navigationPane.push(page)
                }
            } else {
                console.log("Could not create ChatCard.qml")
            }
        }

        function replaceContent(content) {
            if (activeContent) {
                contentHost.remove(activeContent)
                activeContent.destroy()
            }

            activeContent = content
            contentHost.add(activeContent)
        }

        function loadDmList() {
            var page = dmListDefinition.createObject()

            if (page) {
                page.dmSelected.connect(function(channelName) {
                        mainPage.openChat(channelName)
                    })
                replaceContent(page)
            } else {
                console.log("Could not create DmList.qml")
            }
        }

        function loadServerList(serverName) {
            var page = serverListDefinition.createObject()

            if (page) {
                page.serverName = serverName
                page.channelSelected.connect(function(channelName) {
                        mainPage.openChat(channelName)
                    })
                replaceContent(page)
            } else {
                console.log("Could not create ServerList.qml")
            }
        }
        
        onCreationCompleted: {
            serverModel.append({
                    "type": "dm",
                    "name": "Home",
                    "icon": "asset:///images/icons/first.png"
            })
            serverModel.append({
                    "type": "server",
                    "name": "lmao server",
                    "icon": "asset:///images/demo.png"
            })
            serverModel.append({
                    "type": "server",
                    "name": "D",
                    "icon": ""
            })

            loadDmList()
        }
    attachedObjects: [
        ArrayDataModel {
            id: serverModel
        },

        ComponentDefinition {
            id: chatCardDefinition
            source: "asset:///ChatCard.qml"
        },

        ComponentDefinition {
            id: channelMemberListDefinition
            source: "asset:///ChannelMemberList.qml"
        },

        ComponentDefinition {
            id: serverListDefinition
            source: "asset:///ServerList.qml"
        },

        ComponentDefinition {
            id: dmListDefinition
            source: "asset:///DmList.qml"
        }
    ]
}