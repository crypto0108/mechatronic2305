const int trigPin = 9;
const int echoPin = 10;
const int buzzer = 8;
const int bP = 2;

long duracion;
int distancia;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(bP, INPUT_PULLUP); 
  Serial.begin(9600);
}

void loop() {
  if (digitalRead(botonPin) == LOW) { 
    
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duracion = pulseIn(echoPin, HIGH);
    distancia = duracion * 0.034 / 2;

    Serial.print("Distancia: ");
    Serial.println(distancia);

    if (distancia < 50 && distancia > 20) {
      tone(buzzer, 1000); 
      delay(200);
      noTone(buzzer);
      delay(200);
    }
    else if (distancia <= 20) {
      tone(buzzer, 2000);
    }
    else {
      noTone(buzzer); 
    }
    
  } 
  else {
    noTone(buzzer);
  }
}
