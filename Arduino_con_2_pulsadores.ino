void setup()
{
  pinMode(2, INPUT); 
  pinMode(4, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
  int bt1 = digitalRead(2);
  int bt2 = digitalRead(4);
  
  if (bt1 == HIGH || bt2 == HIGH) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  } 
}
