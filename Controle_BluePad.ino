#include <Bluepad32.h>
#include <ESP32Servo.h>

#define leftMotorPin 25 //Motor esquerdo (GPIO 25)
#define rightMotorPin 26 //Motor direito (GPIO 26)
#define BUTTON_PIN 34  // Botão para ativar/desativar Bluetooth (GPIO34)
#define INACTIVITY_TIMEOUT 300000  // 5 minutos sem input antes de desconectar

Servo MotorEsquerdo;
Servo MotorDireito;

ControllerPtr myController = nullptr;
bool bluetoothEnabled = false;  // Flag para controlar Bluetooth
unsigned long lastInputTime = 0;  // Marca hora do último input válido
bool hasParedController = false;  // Verifica se já tem controle pareado
bool isLocked = false;  // TRAVA: Robô travado/desbloqueado com R1
unsigned long lastR1Press = 0;  // Debounce do botão R1

void onConnectedController(ControllerPtr ctl) {
  // Proteção - rejeita novo controle se já tem um pareado
  if (myController == nullptr && hasParedController == false) {
    myController = ctl;
    hasParedController = true;  // Marca que agora tem um pareado
    lastInputTime = millis();   // Reseta o timer de inatividade
    Serial.printf("✓ Controle pareado! Modelo: %s\n", ctl->getModelName().c_str());
    Serial.println("⚠ Apenas 1 controle aceito por vez. Desconecte para trocar.");
  } else if (myController != nullptr && myController != ctl) {
    // Rejeita tentativa de conexão de outro controle
    Serial.println("✗ NEGADO: Outro controle tentou conectar!");
    Serial.println("   Apenas o controle autorizado pode conectar.");
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  if (myController == ctl) {
    myController = nullptr;
    hasParedController = false;  // Permite reconexão do controle
    isLocked = false;             // Reset trava ao desconectar
    Serial.println("⚠ Controle desconectado! Aguardando reconexão...");
    lastInputTime = millis();
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // Configurar botão de controle de Bluetooth
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("\n\n========================================");
  Serial.println("Mini coragem - Controle Seguro");
  Serial.println("========================================");
  Serial.println("✓ Bluetooth SEMPRE ATIVADO");
  Serial.println("→ Conecte o controle PS4");
  Serial.println("→ Só 1 controle PS4 será aceito");
  Serial.println("→ Desconecta auto após 5min sem input");
  Serial.println("→ R1 = Trava de segurança (🔒/🔓)");
  Serial.println("========================================\n");

  // Inicializar BluePad32 mas desabilitado
  BP32.setup(&onConnectedController, &onDisconnectedController);
  bluetoothEnabled = false;

  // Não esquecer as chaves - mantém pareamento seguro
  // BP32.forgetBluetoothKeys();

  // Inicializar motores
  MotorEsquerdo.attach(leftMotorPin);
  MotorDireito.attach(rightMotorPin);
  MotorEsquerdo.write(90);  // Posição neutra
  MotorDireito.write(90);

  lastInputTime = millis();  // Inicializa timer de inatividade corretamente
  Serial.println("Sistema pronto!");
}

void loop() {
  // Verifica se botão foi pressionado (ativa/desativa Bluetooth)
  static unsigned long lastButtonPress = 0;
  if (digitalRead(BUTTON_PIN) == LOW) {  // Botão pressionado (LOW com PULLUP)
    if (millis() - lastButtonPress > 500) {  // Debounce de 500ms
      bluetoothEnabled = !bluetoothEnabled;  // Inverte estado
      lastButtonPress = millis();

      if (bluetoothEnabled) {
        Serial.println("\n✓ BLUETOOTH ATIVADO - Pressione PS4");
        Serial.println("  (Timeout: 5 minutos de inatividade)");
      } else {
        if (myController != nullptr) myController->disconnect();  // Fecha BT de verdade
        myController = nullptr;
        hasParedController = false;  // Permite reconexão ao reativar Bluetooth
        isLocked = false;             // Reset trava ao desativar Bluetooth
        MotorEsquerdo.write(90);
        MotorDireito.write(90);
        Serial.println("\n✗ BLUETOOTH DESATIVADO - Motores parados");
      }
    }
  }

  BP32.update();  // Sempre atualiza para manter estado interno consistente

  // Se Bluetooth desabilitado, não processa inputs
  if (!bluetoothEnabled) {
    delay(50);
    return;
  }

  // Proteção por inatividade - desconecta e para após timeout
  if (myController && myController->isConnected()) {
    if (millis() - lastInputTime > INACTIVITY_TIMEOUT) {
      Serial.println("\n⚠ TIMEOUT! Sem input por 5 minutos.");
      Serial.println("  Desconectando por segurança...");
      bluetoothEnabled = false;
      myController->disconnect();   // Desconecta de verdade via Bluetooth
      myController = nullptr;
      hasParedController = false;  // Permite reconexão após timeout
      MotorEsquerdo.write(90);
      MotorDireito.write(90);
      delay(50);
      return;
    }

    // TRAVA: Verificar botão R1 para ativar/desativar trava de segurança
    if (myController->r1()) {
      if (millis() - lastR1Press > 300) {  // Debounce de 300ms
        isLocked = !isLocked;  // Inverte estado da trava
        lastR1Press = millis();

        if (isLocked) {
          Serial.println("\n🔒 ROBÔ TRAVADO - Pressione R1 para liberar!");
          MotorEsquerdo.write(90);
          MotorDireito.write(90);
        } else {
          Serial.println("\n🔓 ROBÔ LIBERADO - Pronto para competir!");
        }
        lastInputTime = millis();
      }
    }

    // Se robô está travado, ignora joystick
    if (isLocked) {
      delay(50);
      return;
    }

    // Ler valores dos joysticks
    int LStickX = myController->axisX();  // -511 a 512
    int LStickY = myController->axisY();  // -511 a 512

    // Mistura diferencial em valores raw
    int leftRaw  = LStickY - LStickX;
    int rightRaw = LStickY + LStickX;

    // Mapear para 0~180 (neutro = 90)
    int leftMotorOutput  = constrain(map(leftRaw,  -1024, 1024, 0, 180), 0, 180);
    int rightMotorOutput = constrain(map(rightRaw, -1024, 1024, 0, 180), 0, 180);

    // Aplicar zona morta (deadzone)
    if (leftMotorOutput > 95 || leftMotorOutput < 85) {
      MotorEsquerdo.write(leftMotorOutput);
      lastInputTime = millis();  //Atualiza timer de inatividade
    } else {
      MotorEsquerdo.write(90);
    }

    if (rightMotorOutput > 95 || rightMotorOutput < 85) {
      MotorDireito.write(rightMotorOutput);
      lastInputTime = millis();  //Atualiza timer de inatividade
    } else {
      MotorDireito.write(90);
    }

    // Debug
    Serial.print("L:");
    Serial.print(leftMotorOutput);
    Serial.print(" R:");
    Serial.println(rightMotorOutput);

  } else {
    // Parar motores se desconectar
    MotorEsquerdo.write(90);
    MotorDireito.write(90);
  }

  delay(50);
}
