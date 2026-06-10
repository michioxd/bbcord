import bb.cascades 1.4

Sheet {
    id: aboutSheet

    function openLink(url) {
        applicationUI.openLink(url);
    }
    
    Page {
        titleBar: TitleBar {
            title: qsTr("About BBCord")
            dismissAction: ActionItem {
                imageSource: "asset:///images/icons/accent/caret-left.png"
                onTriggered: aboutSheet.close()
            }
        }
        
        ScrollView {
            scrollRole: ScrollRole.Main
            
            Container {
                layout: StackLayout {
    
                }
    
                horizontalAlignment: HorizontalAlignment.Fill
    
                topPadding: ui.du(6.0)
                
                ImageView {
                    imageSource: "asset:///icon.png"
                    preferredWidth: ui.du(24.0)
                    preferredHeight: ui.du(24.0)
                    horizontalAlignment: HorizontalAlignment.Center
                    scalingMethod: ScalingMethod.AspectFit

                    gestureHandlers: [
                        TapHandler {
                            onTapped: aboutSheet.openLink("https://github.com/michioxd/bbcord")
                        }
                    ]
                }
                
                Label {
                    text: "BBCord"
                    textStyle.textAlign: TextAlign.Center
                    horizontalAlignment: HorizontalAlignment.Center
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.fontSize: FontSize.XLarge
                    topMargin: ui.du(0)

                    gestureHandlers: [
                        TapHandler {
                            onTapped: aboutSheet.openLink("https://github.com/michioxd/bbcord")
                        }
                    ]
                }
                
                Label {
                    text: "v" + applicationInfo.version
                    textStyle.textAlign: TextAlign.Center
                    horizontalAlignment: HorizontalAlignment.Center
                    opacity: 0.4
                    topMargin: ui.du(0)
                    textStyle.fontSize: FontSize.XSmall

                    gestureHandlers: [
                        TapHandler {
                            onTapped: aboutSheet.openLink("https://github.com/michioxd/bbcord/releases/tag/" + applicationInfo.version)
                        }
                    ]
                }
                
                Label {
                    text: qsTr("Released under GPL v3.0")
                    textStyle.textAlign: TextAlign.Center
                    horizontalAlignment: HorizontalAlignment.Center
                    opacity: 0.4
                    topMargin: ui.du(0)

                    gestureHandlers: [
                        TapHandler {
                            onTapped: aboutSheet.openLink("https://github.com/michioxd/bbcord/blob/main/LICENSE")
                        }
                    ]
                }
                
                Header {
                    title: qsTr("Created by")
                }
                
                StandardListItem {
                    title: "michioxd"
                    description: "made a whole app?"

                    gestureHandlers: [
                        TapHandler {
                            onTapped: aboutSheet.openLink("https://github.com/michioxd")
                        }
                    ]
                }
                
                Header {
                    title: qsTr("Thanks")
                }
                
                StandardListItem {
                    title: qsTr("BlackBerry (RIM)")
                    description: qsTr("For QNX and BlackBerry 10")

                    gestureHandlers: [
                        TapHandler {
                            onTapped: aboutSheet.openLink("https://www.blackberry.com/")
                        }
                    ]
                }
                
                StandardListItem {
                    title: "Discord"
                    description: "is that a gud platform just to chat?"

                    gestureHandlers: [
                        TapHandler {
                            onTapped: aboutSheet.openLink("https://discord.com/")
                        }
                    ]
                }

                StandardListItem {
                    title: "Oleksandr (cheravoche_02918)"
                    description: "Researched BB10 rooting. Respect his hard work."

                    gestureHandlers: [
                        TapHandler {
                            onTapped: aboutSheet.openLink("https://bb10.root.sx")
                        }
                    ]
                }

                StandardListItem {
                    title: "Mongoose"
                    description: "WebSocket implementation for embedded systems."

                    gestureHandlers: [
                        TapHandler {
                            onTapped: aboutSheet.openLink("https://github.com/cesanta/mongoose")
                        }
                    ]
                }

                StandardListItem {
                    title: "OpenSSL"
                    description: "everyone needs encryption."

                    gestureHandlers: [
                        TapHandler {
                            onTapped: aboutSheet.openLink("https://www.openssl.org/")
                        }
                    ]
                }

                
                Header {
                    title: qsTr("Contributors")
                }
                
                StandardListItem {
                    title: qsTr("You")
                    description: qsTr("Still love and using BB10 right now :3")

                    gestureHandlers: [
                        TapHandler {
                            onTapped: aboutSheet.openLink("https://www.youtube.com/watch?v=dQw4w9WgXcQ")
                        }
                    ]
                }
                
                Label {
                    text: "BBCord is an unofficial Discord client for BlackBerry 10\nIn memory of BlackBerry Mobile (1999–2016)"
                    textStyle.textAlign: TextAlign.Center
                    horizontalAlignment: HorizontalAlignment.Center
                    opacity: 0.5
                    topMargin: ui.du(2.0)
                    textStyle.fontSize: FontSize.XSmall
                    bottomMargin: ui.du(4.0)
                    multiline: true
                }
                
                Container {
                    layoutProperties: StackLayoutProperties {

                    }

                    leftPadding: ui.du(5.0)
                    rightPadding: ui.du(5.0)
                    bottomPadding: ui.du(5.0)
                    horizontalAlignment: HorizontalAlignment.Fill
                    
                    Label {
                        text: "made in Vietnam 🇻🇳 with love :3"
                        textStyle.textAlign: TextAlign.Center
                        horizontalAlignment: HorizontalAlignment.Center
                        opacity: 0.5
                        textStyle.fontSize: FontSize.XXSmall

                        gestureHandlers: [
                            TapHandler {
                                onTapped: aboutSheet.openLink("https://en.wikipedia.org/wiki/Vietnam")
                            }
                        ]
                    }
                    
                    Label {
                        text: "Discord®, the Discord logo, and related marks are trademarks or registered trademarks of Discord Inc.\nBBCord is not affiliated with or endorsed by Discord Inc."
                        textStyle.textAlign: TextAlign.Center
                        horizontalAlignment: HorizontalAlignment.Center
                        opacity: 0.4
                        topMargin: ui.du(1.0)
                        textStyle.fontSize: FontSize.XXSmall
                        bottomMargin: ui.du(4.0)
                        multiline: true
                    }
                }
            }
        }
    }

}
