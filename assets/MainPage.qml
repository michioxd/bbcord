import bb.cascades 1.4

NavigationPane {
    id: nav

    Page {
        id: mainPage
    
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
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill

            layout: StackLayout {}
            
            Container {
                preferredHeight: ui.du(10)
                leftPadding: ui.du(2)
                rightPadding: ui.du(2)
                background: Color.create("#111111")
                horizontalAlignment: HorizontalAlignment.Fill
                
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }
                
                Label {
                    text: "lmao server"
                    verticalAlignment: VerticalAlignment.Center
                    textStyle.fontSize: FontSize.Medium
                    textStyle.fontWeight: FontWeight.Bold
                    horizontalAlignment: HorizontalAlignment.Fill
                }
            }            

            ListView {
                dataModel: channelModel

                onTriggered: {
                    var item = channelModel.data(indexPath)

                    if (item.type == "channel") {
                        var page = chatCardDefinition.createObject()

                        if (page) {
                            page.channelName = item.name
                            page.title = "# " + item.name
                            page.backRequested.connect(function() {
                                    nav.pop()
                                })
                            nav.push(page)
                        } else {
                            console.log("Could not create ChatCard.qml")
                        }
                    }
                }

                listItemComponents: [
                    ListItemComponent {
                        type: "category"

                        Container {
                            preferredHeight: 2.0
                            horizontalAlignment: HorizontalAlignment.Fill
                            leftPadding: ui.du(2)
                            rightPadding: ui.du(2)

                            layout: DockLayout {}

                            topPadding: ui.du(2.0)
                            Label {
                                text: ListItemData.name.toUpperCase()
                                verticalAlignment: VerticalAlignment.Center
                                opacity: 0.45
                                textStyle.fontSize: FontSize.XSmall
                            }
                        }
                    },

                    ListItemComponent {
                        type: "channel"

                        Container {
                            preferredHeight: ui.du(7.0)
                            horizontalAlignment: HorizontalAlignment.Fill
                            leftPadding: ui.du(2)
                            rightPadding: ui.du(2)

                            layout: DockLayout {}

                            Container {
                                layout: StackLayout {
                                    orientation: LayoutOrientation.LeftToRight
                                }
                                
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Center

                                ImageView {
                                    imageSource: "asset:///images/icons/hash.png"

                                    preferredWidth: ui.du(4)
                                    preferredHeight: ui.du(4)

                                    verticalAlignment: VerticalAlignment.Bottom
                                    opacity: 0.45

                                    scalingMethod: ScalingMethod.AspectFit
                                }

                                Label {
                                    text: ListItemData.name
                                    textStyle.fontSize: FontSize.Medium
                                }
                            }
                        }
                    }
                ]

                function itemType(data, indexPath) {
                    return data.type
                }
            }
        }
    }

    attachedObjects: [
        Sheet {
            id: userSheet

            Page {
                titleBar: TitleBar {
                    title: "User"
                }

                Container {
                    Label {
                        text: "User menu"
                    }

                    Button {
                        text: "Close"
                        onClicked: userSheet.close()
                    }
                }
            }
        },
        ArrayDataModel {
            id: serverModel
        },
        
        ArrayDataModel {
            id: channelModel
        }
    ]
    
    onCreationCompleted: {
        serverModel.append({
                "name": "Home",
                "icon": "asset:///images/icons/house.png"
        })
        serverModel.append({
                "name": "D",
                "icon": "asset:///images/demo.png"
        })
        serverModel.append({
                "name": "D",
                "icon": ""
        })

        channelModel.append({"type": "category", "name": "Text Channels"})
        channelModel.append({"type": "channel", "name": "general"})
        channelModel.append({"type": "channel", "name": "random"})
        channelModel.append({"type": "channel", "name": "dev"})

        channelModel.append({"type": "category", "name": "Voice Channels"})
        channelModel.append({"type": "channel", "name": "lounge"})
        channelModel.append({"type": "channel", "name": "music"})
    }

    }

    Menu.definition: MenuDefinition {
        actions: [
            ActionItem {
                title: "User"
                imageSource: "asset:///images/icon.png"

                onTriggered: {
                    userSheet.open()
                }
            },

            ActionItem {
                title: "Log out"
                imageSource: "asset:///images/icons/sign-out.png"

                onTriggered: {
                    console.log("logging out")
                }
            },

            ActionItem {
                title: "Settings"

                onTriggered: {
                    console.log("settings")
                }
            }
        ]
    }

    onPopTransitionEnded: {
        page.destroy()
    }

    attachedObjects: [
        ComponentDefinition {
            id: chatCardDefinition
            source: "asset:///ChatCard.qml"
        }
    ]
}