import bb.cascades 1.4

Page {
	id: memberPage

	property string channelName: "general"
	property alias title: titleBar.title

	signal backRequested()

	actionBarVisibility: ChromeVisibility.Hidden

	titleBar: TitleBar {
		id: titleBar
		title: qsTr("Members #") + memberPage.channelName
		visibility: ChromeVisibility.Visible

		dismissAction: ActionItem {
			imageSource: "asset:///images/icons/accent/caret-left.png"

			onTriggered: {
				memberPage.backRequested()
			}
		}
	}

	Container {
		horizontalAlignment: HorizontalAlignment.Fill
		verticalAlignment: VerticalAlignment.Fill

		layout: StackLayout {}

		ListView {
			id: memberList
			dataModel: memberModel
			horizontalAlignment: HorizontalAlignment.Fill
			verticalAlignment: VerticalAlignment.Fill

			listItemComponents: [
				ListItemComponent {
					type: "role"

					Container {
						horizontalAlignment: HorizontalAlignment.Fill
						leftPadding: ui.du(2.0)
						rightPadding: ui.du(2.0)
						topPadding: ui.du(2.2)
						bottomPadding: ui.du(0.6)

						Label {
							text: ListItemData.name + " — " + ListItemData.count
							opacity: 0.55
							textStyle.fontSize: FontSize.XSmall
							textStyle.fontWeight: FontWeight.Normal
							textStyle.color: Color.create("#B5BAC1")
						}
					}
				},

				ListItemComponent {
					type: "member"

					Container {
						horizontalAlignment: HorizontalAlignment.Fill
						leftPadding: ui.du(2.0)
						rightPadding: ui.du(2.0)
						topPadding: ui.du(0.8)
						bottomPadding: ui.du(0.8)

						layout: StackLayout {
							orientation: LayoutOrientation.LeftToRight
						}

						Container {
							preferredWidth: ui.du(7.0)
							preferredHeight: ui.du(7.0)
							minWidth: ui.du(7.0)
							minHeight: ui.du(7.0)
							maxWidth: ui.du(7.0)
							maxHeight: ui.du(7.0)
							verticalAlignment: VerticalAlignment.Center
							background: Color.create(ListItemData.avatarColor)

							layout: DockLayout {}

							ImageView {
								imageSource: ListItemData.avatar
								visible: ListItemData.avatar !== ""
								horizontalAlignment: HorizontalAlignment.Fill
								verticalAlignment: VerticalAlignment.Fill
								scalingMethod: ScalingMethod.AspectFill
							}

							Label {
								text: ListItemData.initials
								visible: ListItemData.avatar === ""
								horizontalAlignment: HorizontalAlignment.Center
								verticalAlignment: VerticalAlignment.Center
								textStyle.fontSize: FontSize.Small
								textStyle.fontWeight: FontWeight.Bold
								textStyle.color: Color.White
							}
						}

						Container {
							horizontalAlignment: HorizontalAlignment.Fill
							verticalAlignment: VerticalAlignment.Center
							leftMargin: ui.du(1.5)

							Label {
								text: ListItemData.name
								textStyle.fontSize: FontSize.Small
								textStyle.fontWeight: FontWeight.Normal
								textStyle.color: Color.create(ListItemData.nameColor)
							}

							Label {
								text: ListItemData.status
								topMargin: ui.du(-0.4)
								opacity: 0.6
								textStyle.fontSize: FontSize.XSmall
								textStyle.color: Color.create("#B5BAC1")
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

	attachedObjects: [
		ArrayDataModel {
			id: memberModel
		}
	]

	function addRole(name, count) {
		memberModel.append({
				"type": "role",
				"name": name,
				"count": count
			})
	}

	function addMember(name, initials, avatarColor, avatar, nameColor, status) {
		memberModel.append({
				"type": "member",
				"name": name,
				"initials": initials,
				"avatarColor": avatarColor,
				"avatar": avatar,
				"nameColor": nameColor,
				"status": status
			})
	}

	onCreationCompleted: {
		addRole("Owner", 1)
		addMember("michioxd", "M", "#5865F2", "", "#F2F3F5", "Online")

		addRole("Bots", 1)
		addMember("check", "C", "#43B581", "", "#43B581", "please check back soon :3")
		addMember("back", "B", "#43B581", "", "#43B581", "please check back soon :3")
		addMember("later", "L", "#43B581", "", "#43B581", "please check back soon :3")
	}
}
