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
                id: guildListView
                dataModel: serverModel
                verticalAlignment: VerticalAlignment.Fill

                function loadVisibleGuildIcon(guildId) {
                    mainPageController.loadGuildIcon(guildId);
                }

                onTriggered: {
                    var item = serverModel.data(indexPath);

                    if (item.type == "dm") {
                        mainPageController.selectHome();
                        mainPage.loadDmList();
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

                            topMargin: ui.du(1)
                            bottomMargin: ui.du(1)

                            background: Color.create("#333333")

                            layout: DockLayout {}

                            Container {
                                preferredWidth: ui.du(1.0)
                                preferredHeight: ui.du(4.0)
                                horizontalAlignment: HorizontalAlignment.Left
                                verticalAlignment: VerticalAlignment.Center
                                background: Color.create("#FFFFFF")
                                visible: ListItemData.type == "server" && ListItemData.unread == true
                            }

                            function requestIcon() {
                                if (ListItemData.type == "server" && ListItemData.icon === "" && ListItemData.iconHash !== "") {
                                    ListItem.view.loadVisibleGuildIcon(ListItemData.id);
                                }
                            }

                            onCreationCompleted: {
                                requestIcon();
                            }

                            ListItem.onDataChanged: {
                                requestIcon();
                            }

                            ImageView {
                                imageSource: ListItemData.icon
                                visible: ListItemData.icon !== ""
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                scalingMethod: ScalingMethod.AspectFit
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

    function openChat(channelName) {
        var page = chatCardDefinition.createObject();

        if (page) {
            page.channelName = channelName;
            page.title = "# " + channelName;
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
        var page = dmListDefinition.createObject();

        if (page) {
            page.dmSelected.connect(function (channelName) {
                mainPage.openChat(channelName);
            });
            replaceContent(page);
        } else {
            console.log("Could not create DmList.qml");
        }
    }

    function loadServerList(serverId, serverName) {
        var page = serverListDefinition.createObject();

        if (page) {
            page.serverId = serverId;
            page.serverName = serverName;
            page.channelSelected.connect(function (channelName) {
                mainPage.openChat(channelName);
            });
            replaceContent(page);
        } else {
            console.log("Could not create ServerList.qml");
        }
    }

    onCreationCompleted: {
        resetServers();

        loadDmList();
        appStore.guildIconChanged.connect(updateServerIcon);
        appStore.guildsChanged.connect(refreshServersIfNeeded);
        mainPageController.loadMainData();
    }

    function resetServers() {
        serverModel.clear();
        serverModel.append({
            "type": "dm",
            "name": "Home",
            "id": "home",
            "icon": "asset:///images/icons/first.png",
            "mentionCount": 0,
            "unread": false
        });

        for (var i = 0; i < appStore.guilds.length; ++i) {
            serverModel.append(appStore.guilds[i]);
        }
    }

    function refreshServersIfNeeded() {
        if (serverModel.size() - 1 != appStore.guilds.length) {
            resetServers();
            return;
        }

        for (var i = 0; i < appStore.guilds.length; ++i) {
            var item = serverModel.data([i + 1]);
            if (item.id != appStore.guilds[i].id) {
                resetServers();
                return;
            }

            if (item.icon != appStore.guilds[i].icon || item.mentionCount != appStore.guilds[i].mentionCount || item.unread != appStore.guilds[i].unread) {
                serverModel.replace([i + 1], appStore.guilds[i]);
            }
        }
    }

    function updateServerIcon(guildId, iconSource) {
        for (var i = 1; i < serverModel.size(); ++i) {
            var item = serverModel.data([i]);
            if (item.id == guildId) {
                item.icon = iconSource;
                serverModel.replace([i], item);
                break;
            }
        }
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
