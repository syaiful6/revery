open Revery;
open Revery.UI;
open Revery.UI.Components;

let bgColor = Color.hex("#212733");

let render = () => {
  let cardStyle = [
    Style.backgroundColor(Color.rgba(1.0, 1.0, 1.0, 0.1)),
    Style.borderRadius(8.0),
    Style.padding(16),
    Style.margin(8),
    Style.width(200),
  ];

  let buttonStyle = [
    Style.backgroundColor(Color.hex("#007AFF")),
    Style.borderRadius(6.0),
    Style.paddingHorizontal(16),
    Style.paddingVertical(8),
  ];

  <View
    style=Style.[
      position(`Absolute),
      top(0),
      bottom(0),
      left(0),
      right(0),
      backgroundColor(bgColor),
      padding(20),
    ]>
    <Column>
      <Column>
        <Text fontSize=32.0 text="Layout Demo with Existing Components" />
        <Text fontSize=16.0 text="Using Row, Column, Center, and Spacer" />
      </Column>
      <Spacer />
      <Column>
        <Text fontSize=20.0 text="Card Layout with Row" />
        <Row>
          <View style=cardStyle>
            <Column>
              <Text fontSize=18.0 text="Card 1" />
              <Text fontSize=14.0 text="First card content." />
              <Spacer />
              <Clickable
                style=buttonStyle
                onClick={_ => print_endline("Card 1 clicked!")}>
                <Text fontSize=14.0 text="Action" />
              </Clickable>
            </Column>
          </View>
          <View style=cardStyle>
            <Column>
              <Text fontSize=18.0 text="Card 2" />
              <Text fontSize=14.0 text="Second card content." />
              <Spacer />
              <Clickable
                style=buttonStyle
                onClick={_ => print_endline("Card 2 clicked!")}>
                <Text fontSize=14.0 text="Action" />
              </Clickable>
            </Column>
          </View>
          <View style=cardStyle>
            <Column>
              <Text fontSize=18.0 text="Card 3" />
              <Text fontSize=14.0 text="Third card content." />
              <Spacer />
              <Clickable
                style=buttonStyle
                onClick={_ => print_endline("Card 3 clicked!")}>
                <Text fontSize=14.0 text="Action" />
              </Clickable>
            </Column>
          </View>
        </Row>
      </Column>
      <Spacer />
      <Column>
        <Text fontSize=20.0 text="Toolbar Layout with Spacer" />
        <View
          style=Style.[
            backgroundColor(Color.rgba(1.0, 1.0, 1.0, 0.05)),
            borderRadius(8.0),
            padding(12),
          ]>
          <Row>
            <Text fontSize=16.0 text="My App" />
            <Spacer />
            <Row>
              <Clickable
                style=Style.[
                  backgroundColor(Color.rgba(1.0, 1.0, 1.0, 0.1)),
                  borderRadius(4.0),
                  padding(8),
                  margin(4),
                ]
                onClick={_ => print_endline("Settings clicked!")}>
                <Text fontSize=14.0 text="Settings" />
              </Clickable>
              <Clickable
                style=Style.[
                  backgroundColor(Color.hex("#007AFF")),
                  borderRadius(4.0),
                  padding(8),
                  margin(4),
                ]
                onClick={_ => print_endline("Profile clicked!")}>
                <Text fontSize=14.0 text="Profile" />
              </Clickable>
            </Row>
          </Row>
        </View>
      </Column>
      <Spacer />
      <Row>
        <Text
          fontSize=12.0
          text="Built with Revery's Row, Column, and Spacer"
        />
      </Row>
    </Column>
  </View>;
};
