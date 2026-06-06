import bb.cascades 1.4

Page {
    Container {
        layout: DockLayout {}
        
        Container {
            horizontalAlignment: HorizontalAlignment.Center
            verticalAlignment: VerticalAlignment.Center

            layout: StackLayout {
            }

            leftPadding: 50.0
            rightPadding: 50.0

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