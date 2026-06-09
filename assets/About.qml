import bb.cascades 1.4
import bb.system 1.2

Sheet {
    id: aboutSheet
    
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
                }
                
                Label {
                    text: "BBCord"
                    textStyle.textAlign: TextAlign.Center
                    horizontalAlignment: HorizontalAlignment.Center
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.fontSize: FontSize.XLarge
                    topMargin: ui.du(0)
                }
                
                Label {
                    text: "v0.x.x"
                    textStyle.textAlign: TextAlign.Center
                    horizontalAlignment: HorizontalAlignment.Center
                    opacity: 0.4
                    topMargin: ui.du(0)
                    textStyle.fontSize: FontSize.XSmall
                }
                
                Label {
                    text: qsTr("Released under GPL v3.0")
                    textStyle.textAlign: TextAlign.Center
                    horizontalAlignment: HorizontalAlignment.Center
                    opacity: 0.4
                    topMargin: ui.du(0)
                    
                }
                
                Header {
                    title: qsTr("Created by")
                }
                
                StandardListItem {
                    title: "michioxd"
                    description: "made a whole app?"
                }
                
                Header {
                    title: qsTr("Thanks")
                }
                
                StandardListItem {
                    title: qsTr("BlackBerry (RIM)")
                    description: qsTr("For QNX and BlackBerry 10")
                }
                
                StandardListItem {
                    title: "Discord"
                    description: "is that a gud platform just to chat?"
                }

                StandardListItem {
                    title: "Oleksandr (cheravoche_02918)"
                    description: "Researched BB10 rooting. Respect his hard work."
                }

                StandardListItem {
                    title: "cesanta's mongoose"
                    description: "WebSocket implementation for embedded systems."
                }

                StandardListItem {
                    title: "OpenSSL"
                    description: "everyone needs encryption."
                }

                
                Header {
                    title: qsTr("Contributors")
                }
                
                StandardListItem {
                    title: qsTr("You")
                    description: qsTr("Still love and using BB10 right now :3")
                }
                
                Label {
                    text: "made in Vietnam 🇻🇳 with love :3"
                    textStyle.textAlign: TextAlign.Center
                    horizontalAlignment: HorizontalAlignment.Center
                    opacity: 0.5
                    topMargin: ui.du(2.0)
                    textStyle.fontSize: FontSize.XXSmall
                    bottomMargin: ui.du(4.0)
                }
                
                Label {
                    text: ""
                }
            }
        }
    }
}
