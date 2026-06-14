const byte PB = 2; 

void setup(); {
  pinMode(PB, INPUT); 
  pinMode(LED_BUILTIN, OUTPUT); 
}

void loop(); {
  if (digitalRead(PB) == HIGH) { 
    digitalWrite(LED_BUILTIN, HIGH); 
  } else { 
    digitalWrite(LED_BUILTIN, LOW); 
  } 
} 
