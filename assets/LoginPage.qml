import bb.cascades 1.4

Page {
    signal loginSucceeded()

    Container {
        layout: DockLayout {}
        
        Container {
            horizontalAlignment: HorizontalAlignment.Center
            verticalAlignment: VerticalAlignment.Center

            layout: StackLayout {
            }

            leftPadding: ui.du(8.0)
            rightPadding: ui.du(8.0)

            ImageView {
                imageSource: "asset:///images/icon.png"
                horizontalAlignment: HorizontalAlignment.Center
                scalingMethod: ScalingMethod.AspectFit
            }

            Label {
                text: qsTr("Welcome to BBCord")
                horizontalAlignment: HorizontalAlignment.Center
            }

            TextField {
                id: tokenField
                hintText: qsTr("Enter your Discord token...")
                horizontalAlignment: HorizontalAlignment.Fill
                textFormat: TextFormat.Plain
                inputMode: TextFieldInputMode.Text
                text: ""
            }

            Button {
                id: btnLogin

                text: qsTr("Login")
                horizontalAlignment: HorizontalAlignment.Fill
                enabled: !discordClient.busy

                onClicked: {
                    discordClient.login(tokenField.text)
                }
            }

            Button {
                id: btnContinue

                text: qsTr("Continue")
                horizontalAlignment: HorizontalAlignment.Fill
                visible: false

                onClicked: {
                    console.log("[login] continue clicked")
                    loginSucceeded()
                }
            }

            Label {
                text: discordClient.statusText
                horizontalAlignment: HorizontalAlignment.Center
                multiline: true
                visible: text.length > 0
            }

            Button {
                id: btnInst
                text: qsTr("How to get token")
                horizontalAlignment: HorizontalAlignment.Fill
            }

            Label {
                text: "powered by michioxd"
                horizontalAlignment: HorizontalAlignment.Center
                textStyle.fontSizeValue: 0.0
                textFit.minFontSizeValue: 3.0
                textFit.maxFontSizeValue: 4.0
                textFit.mode: LabelTextFitMode.Default
                opacity: 0.5
            }
        }
    }
}