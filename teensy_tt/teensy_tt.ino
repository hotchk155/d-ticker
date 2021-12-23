void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}




void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(analogRead(A0));
  Serial.println(analogRead(A1));
  Serial.println(analogRead(A2));
  Serial.println(analogRead(A3));
  Serial.println("===");
  delay(500);  
}
