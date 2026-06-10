import bb.cascades 1.4

Page {
    titleBar: TitleBar {
        title: qsTr("Settings")
    }

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill

        ScrollView {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill

            Container {
                horizontalAlignment: HorizontalAlignment.Fill

                Header {
                    title: qsTr("Miscellaneous")
                }

                CustomListItem {
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        leftPadding: ui.du(2.0)
                        rightPadding: ui.du(2.0)

                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }

                        Container {
                            leftMargin: ui.du(2.0)
                            verticalAlignment: VerticalAlignment.Center
                            horizontalAlignment: HorizontalAlignment.Fill

                            layoutProperties: StackLayoutProperties {
                                spaceQuota: 1
                            }

                            Label {
                                text: qsTr("Sound effects")
                                textStyle.fontSize: FontSize.Large
                                horizontalAlignment: HorizontalAlignment.Fill
                                bottomMargin: ui.du(0)
                            }

                            Label {
                                text: qsTr("Play login, connecting, and error sounds. #MSNTV2")
                                opacity: 0.55
                                topMargin: ui.du(0)
                                bottomMargin: ui.du(0)
                                textStyle.fontSize: FontSize.Small
                                multiline: true
                            }
                        }
                        
                        ToggleButton {
                            id: sfxSwitch
                            checked: settingsController.sfxEnabled
                            verticalAlignment: VerticalAlignment.Center

                            layoutProperties: StackLayoutProperties {
                                spaceQuota: -1
                            }
                            
                            onCheckedChanged: {
                                settingsController.setSfxEnabled(checked)
                            }
                        }
                    }
                }
            }
        }
    }
}
