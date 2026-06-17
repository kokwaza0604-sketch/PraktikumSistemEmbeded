// Deklarasi pin
const int button1 = 12;
const int button2 = 14;
const int led1 = 26;
const int led2 = 27;

void setup() {
  Serial.begin(115200);

  // push button sebagai input
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);

  // LED sebagai output
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
}

void loop() {

  int stateBtn1 = digitalRead(button1);
  int stateBtn2 = digitalRead(button2);

  Serial.print("Button1: ");
  Serial.print(stateBtn1);
  Serial.print(" | Button2: ");
  Serial.println(stateBtn2);

  if (Serial.available() > 0) {
    char data = Serial.read();

    if (data == '1') {
      digitalWrite(led1, HIGH);
    }
    else if (data == '2') {
      digitalWrite(led1, LOW);
    }
    else if (data == '3') {
      digitalWrite(led2, HIGH);
    }
    else if (data == '4') {
      digitalWrite(led2, LOW);
    }
  }

  delay(200);
}