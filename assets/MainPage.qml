import bb.cascades 1.4

Page {
    id: mainPage

    property variant navigationPane: null
    property variant activeContent: null
    property string activeContentType: ""
    property string activeServerId: ""

    Container {
        layout: StackLayout {
            orientation: LayoutOrientation.LeftToRight
        }

        Container {
            preferredWidth: ui.du(12)
            verticalAlignment: VerticalAlignment.Fill
            background: Color.create("#111111")

            ListView {
                id: guildListView
                dataModel: mainPageController.serverDataModel
                verticalAlignment: VerticalAlignment.Fill

                function loadVisibleGuildIcon(guildId) {
                    mainPageController.loadGuildIcon(guildId);
                }

                onTriggered: {
                    var item = mainPageController.serverDataModel.data(indexPath);

                    if (item.type == "dm") {
                        mainPage.loadDmList();
                        mainPageController.selectHome();
                    } else if (item.type == "server") {
                        mainPageController.selectGuild(item.id);
                        mainPage.loadServerList(item.id, item.name);
                    }
                }

                listItemComponents: [
                    ListItemComponent {
                        type: ""

                        Container {
                            preferredWidth: ui.du(10)
                            preferredHeight: ui.du(10)
                            property string lastIconRequestId: ""
                            property bool selectedItem: ListItemData.active == true
                            property bool selectedServer: selectedItem && ListItemData.type == "server"

                            topMargin: ui.du(1)
                            bottomMargin: ui.du(1)

                            background: selectedItem ? Color.create("#5865F2") : Color.create("#333333")

                            layout: DockLayout {}

                            function requestIcon() {
                                if (lastIconRequestId == ListItemData.id) {
                                    return;
                                }
                                if (ListItemData.type == "server" && ListItemData.icon === "" && ListItemData.iconHash !== "") {
                                    lastIconRequestId = ListItemData.id;
                                    ListItem.view.loadVisibleGuildIcon(ListItemData.id);
                                }
                            }

                            onCreationCompleted: {
                                requestIcon();
                            }

                            ListItem.onDataChanged: {
                                if (lastIconRequestId != ListItemData.id) {
                                    requestIcon();
                                }
                            }

                            Container {
                                visible: ListItemData.icon !== ""
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                topPadding: selectedServer ? ui.du(0.5) : 0
                                bottomPadding: selectedServer ? ui.du(0.5) : 0
                                leftPadding: selectedServer ? ui.du(0.5) : 0
                                rightPadding: selectedServer ? ui.du(0.5) : 0

                                layout: DockLayout {}

                                ImageView {
                                    imageSource: ListItemData.icon
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    verticalAlignment: VerticalAlignment.Fill
                                    scalingMethod: ScalingMethod.AspectFit
                                }
                            }

                            Label {
                                text: ListItemData.initials

                                visible: ListItemData.icon === ""

                                horizontalAlignment: HorizontalAlignment.Center
                                verticalAlignment: VerticalAlignment.Center

                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.fontSize: FontSize.Large
                            }

                            Container {
                                preferredWidth: ui.du(1.0)
                                preferredHeight: ui.du(4.0)
                                horizontalAlignment: HorizontalAlignment.Left
                                verticalAlignment: VerticalAlignment.Center
                                background: Color.create("#FFFFFF")
                                visible: ListItemData.type == "server" && ListItemData.unread == true
                            }

                            Container {
                                preferredWidth: ui.du(3)
                                preferredHeight: ui.du(3)
                                horizontalAlignment: HorizontalAlignment.Right
                                verticalAlignment: VerticalAlignment.Top
                                background: Color.create("#ED4245")
                                visible: ListItemData.mentionCount > 0

                                layout: DockLayout {}

                                Label {
                                    text: ListItemData.mentionCount > 99 ? "99+" : ListItemData.mentionCount
                                    horizontalAlignment: HorizontalAlignment.Center
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.color: Color.White
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.fontSize: FontSize.XXSmall
                                }
                            }
                        }
                    }
                ]

                function itemType(data, indexPath) {
                    return "";
                }

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

    function openChat(channelId, guildId, channelName) {
        var page = chatCardDefinition.createObject();

        if (page) {
            chatController.openChannel(channelId, guildId, channelName);
            page.channelName = channelName;
            page.title = channelName;
            page.compactMessageEnabled = settingsController.compactMessageEnabled;
            settingsController.compactMessageEnabledChanged.connect(function (enabled) {
                page.compactMessageEnabled = enabled;
            });
            page.backRequested.connect(function () {
                if (mainPage.navigationPane) {
                    mainPage.navigationPane.pop();
                }
            });
            page.memberListRequested.connect(function () {
                var memberPage = channelMemberListDefinition.createObject();

                if (memberPage) {
                    memberPage.channelName = channelName;
                    memberPage.title = "Members #" + channelName;
                    memberPage.backRequested.connect(function () {
                        if (mainPage.navigationPane) {
                            mainPage.navigationPane.pop();
                        }
                    });
                    if (mainPage.navigationPane) {
                        mainPage.navigationPane.push(memberPage);
                    }
                } else {
                    console.log("Could not create ChannelMemberList.qml");
                }
            });
            if (mainPage.navigationPane) {
                mainPage.navigationPane.push(page);
            }
        } else {
            console.log("Could not create ChatCard.qml");
        }
    }

    function replaceContent(content) {
        if (activeContent) {
            contentHost.remove(activeContent);
            activeContent.destroy();
        }

        activeContent = content;
        contentHost.add(activeContent);
    }

    function loadDmList() {
        mainPageController.selectHome();

        if (activeContentType == "dm") {
            return;
        }

        var page = dmListDefinition.createObject();

        if (page) {
            page.dmSelected.connect(function (channelId, channelName) {
                mainPage.openChat(channelId, "", channelName);
            });
            replaceContent(page);
            activeContentType = "dm";
            activeServerId = "";
        } else {
            console.log("Could not create DmList.qml");
        }
    }

    function loadServerList(serverId, serverName) {
        if (activeContentType == "server" && activeServerId == serverId) {
            return;
        }

        var page = serverListDefinition.createObject();

        if (page) {
            page.serverId = serverId;
            page.serverName = serverName;
            page.channelSelected.connect(function (channelId, guildId, channelName) {
                mainPage.openChat(channelId, guildId, channelName);
            });
            replaceContent(page);
            activeContentType = "server";
            activeServerId = serverId;
        } else {
            console.log("Could not create ServerList.qml");
        }
    }

    onCreationCompleted: {
        loadDmList();
        mainPageController.loadMainData();
    }

    attachedObjects: [
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
