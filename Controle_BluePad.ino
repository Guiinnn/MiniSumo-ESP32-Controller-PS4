#include <Bluepad32.h>

// PINOS DO TB6612FNG
// Motor A = Motor Esquerdo | Motor B = Motor Direito
#define AIN1 27   // Direção motor esquerdo (A)
#define AIN2 14   // Direção motor esquerdo (A)
#define PWMA 25   // PWM (velocidade) motor esquerdo (A)

#define BIN1 32   // Direção motor direito (B)
#define BIN2 33   // Direção motor direito (B)
#define PWMB 26   // PWM (velocidade) motor direito (B)

#define STBY 13   // Standby do driver (precisa ficar HIGH pra ligar)

#define BUTTON_PIN 34  // Botão para ativar/desativar Bluetooth (GPIO34)
#define INACTIVITY_TIMEOUT 300000  // 5 minutos sem input antes de desconectar

// Canais PWM (LEDC) do ESP32
#define PWMA_CHANNEL 0
#define PWMB_CHANNEL 1
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8   // 8 bits valores de 0 a 255

ControllerPtr myController = nullptr;
bool bluetoothEnabled = false;
unsigned long lastInputTime = 0;
bool hasParedController = false;
bool isLocked = false;
unsigned long lastR1Press = 0;

// FUNÇÃO DE CONTROLE DO MOTOR
// speed: -255 (ré máxima) a 255 (frente máxima). 0 = parado.
void driveMotor(int in1Pin, int in2Pin, int pwmChannel, int speed) {
  speed = constrain(speed, -255, 255);

  if (speed > 0) {
    digitalWrite(in1Pin, HIGH);
    digitalWrite(in2Pin, LOW);
  } else if (speed < 0) {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, HIGH);
  } else {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, LOW);
  }

  ledcWrite(pwmChannel, abs(speed));
}

void stopMotors() {
  driveMotor(AIN1, AIN2, PWMA_CHANNEL, 0);
  driveMotor(BIN1, BIN2, PWMB_CHANNEL, 0);
}

// CALLBACKS BLUEPAD32
void onConnectedController(ControllerPtr ctl) {
  if (myController == nullptr && hasParedController == false) {
    myController = ctl;
    hasParedController = true;
    lastInputTime = millis();
    Serial.printf("Controle pareado! Modelo: %s\n", ctl->getModelName().c_str());
    Serial.println("Apenas 1 controle aceito por vez. Desconecte para trocar.");
  } else if (myController != nullptr && myController != ctl) {
    Serial.println("NEGADO: Outro controle tentou conectar!");
    Serial.println("   Apenas o controle autorizado pode conectar.");
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  if (myController == ctl) {
    myController = nullptr;
    hasParedController = false;
    isLocked = false;
    Serial.println("Controle desconectado! Aguardando reconexão...");
    lastInputTime = millis();
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("\n\n========================================");
  Serial.println("Mini coragem - Controle Seguro (TB6612FNG)");
  Serial.println("========================================");
  Serial.println("Bluetooth SEMPRE ATIVADO");
  Serial.println("Conecte o controle PS4");
  Serial.println("Só 1 controle PS4 será aceito");
  Serial.println("Desconecta auto após 5min sem input");
  Serial.println("R1 = Trava de segurança");
  Serial.println("========================================\n");

  BP32.setup(&onConnectedController, &onDisconnectedController);
  bluetoothEnabled = false;

  // ---- Configuração dos pinos do TB6612FNG ----
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(STBY, OUTPUT);

  ledcSetup(PWMA_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PWMA, PWMA_CHANNEL);
  ledcSetup(PWMB_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PWMB, PWMB_CHANNEL);

  digitalWrite(STBY, HIGH);  // Tira o driver do modo standby
  stopMotors();

  lastInputTime = millis();
  Serial.println("Sistema pronto!");
}

void loop() {
  static unsigned long lastButtonPress = 0;
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (millis() - lastButtonPress > 500) {
      bluetoothEnabled = !bluetoothEnabled;
      lastButtonPress = millis();

      if (bluetoothEnabled) {
        Serial.println("\nBLUETOOTH ATIVADO - Pressione PS4");
        Serial.println("  (Timeout: 5 minutos de inatividade)");
      } else {
        if (myController != nullptr) myController->disconnect();
        myController = nullptr;
        hasParedController = false;
        isLocked = false;
        stopMotors();
        Serial.println("\nBLUETOOTH DESATIVADO - Motores parados");
      }
    }
  }

  BP32.update();

  if (!bluetoothEnabled) {
    delay(50);
    return;
  }

  if (myController && myController->isConnected()) {
    if (millis() - lastInputTime > INACTIVITY_TIMEOUT) {
      Serial.println("\nTIMEOUT! Sem input por 5 minutos.");
      Serial.println("  Desconectando por segurança...");
      bluetoothEnabled = false;
      myController->disconnect();
      myController = nullptr;
      hasParedController = false;
      stopMotors();
      delay(50);
      return;
    }

    if (myController->r1()) {
      if (millis() - lastR1Press > 300) {
        isLocked = !isLocked;
        lastR1Press = millis();

        if (isLocked) {
          Serial.println("\nROBÔ TRAVADO - Pressione R1 para liberar!");
          stopMotors();
        } else {
          Serial.println("\nROBÔ LIBERADO - Pronto para competir!");
        }
        lastInputTime = millis();
      }
    }

    if (isLocked) {
      delay(50);
      return;
    }

    int LStickX = myController->axisX();  // -511 a 512
    int LStickY = myController->axisY();  // -511 a 512

    int leftRaw  = LStickY - LStickX;
    int rightRaw = LStickY + LStickX;

    int leftSpeed  = constrain(map(leftRaw,  -1024, 1024, -255, 255), -255, 255);
    int rightSpeed = constrain(map(rightRaw, -1024, 1024, -255, 255), -255, 255);

    const int DEADZONE = 20;

    if (abs(leftSpeed) > DEADZONE) {
      driveMotor(AIN1, AIN2, PWMA_CHANNEL, leftSpeed);
      lastInputTime = millis();
    } else {
      driveMotor(AIN1, AIN2, PWMA_CHANNEL, 0);
    }

    if (abs(rightSpeed) > DEADZONE) {
      driveMotor(BIN1, BIN2, PWMB_CHANNEL, rightSpeed);
      lastInputTime = millis();
    } else {
      driveMotor(BIN1, BIN2, PWMB_CHANNEL, 0);
    }

    Serial.print("L:");
    Serial.print(leftSpeed);
    Serial.print(" R:");
    Serial.println(rightSpeed);

  } else {
    stopMotors();
  }

  delay(50);
}