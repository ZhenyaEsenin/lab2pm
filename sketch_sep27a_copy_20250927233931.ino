// --- Пины светодиодов и кнопок ---
const int R=13,Y=12,G=11,BTN=2,EMERG=3;

// --- Состояния светофора ---
enum St{GREEN,YELLOW,RED,RED_BLINK,WARNING,NIGHT}; 
St st; // текущее состояние

// --- Переменные для таймеров и состояний ---
unsigned long t0,dur,lastBtn=0;
bool ped=0,night=0,btn=0,prevEmerg=0;
int blinkCnt=0;

// --- Функция управления светодиодами ---
void leds(int r,int y,int g){
  digitalWrite(R,r);
  digitalWrite(Y,y);
  digitalWrite(G,g);
}

// --- Таблица состояний светофора ---
struct StateConfig {
  int r, y, g;           // состояние светодиодов
  unsigned long duration; // длительность состояния
  St nextState;          // следующее состояние по таймеру
  void (*action)(void);  // действие при переходе
  void (*update)(void);  // действие при обновлении (для особых состояний)
};

// --- Действия для особых состояний ---
void redBlinkAction() { blinkCnt = 6; }
void warningUpdate() { 
  digitalWrite(Y, !digitalRead(Y));
  t0 = millis();
}
void nightUpdate() { 
  digitalWrite(Y, !digitalRead(Y));
  t0 = millis();
}
void redBlinkUpdate() { 
  digitalWrite(R, !digitalRead(R));
  blinkCnt--;
  t0 = millis();
}
void noAction() {}
void noUpdate() {}

// --- Таблица конфигурации состояний ---
const StateConfig stateTable[] = {
  // r, y, g, duration, nextState, action, update
  {0, 0, 1, 10000, YELLOW, noAction, noUpdate},    // GREEN
  {0, 1, 0,  3000, RED,    noAction, noUpdate},    // YELLOW
  {1, 0, 0, 10000, GREEN,  noAction, noUpdate},    // RED
  {0, 0, 0,   500, GREEN,  redBlinkAction, redBlinkUpdate}, // RED_BLINK
  {0, 0, 0,   500, WARNING, noAction, warningUpdate}, // WARNING
  {0, 0, 0,  1000, NIGHT,   noAction, nightUpdate}   // NIGHT
};

// --- Проверка особых состояний ---
bool isSpecialState(St s) {
  return (s == RED_BLINK || s == WARNING || s == NIGHT);
}

// --- Переход в новое состояние ---
void next(St s, unsigned long customDur = 0){
  st = s;
  t0 = millis();
  dur = (customDur > 0) ? customDur : stateTable[s].duration;
  
  leds(0, 0, 0); // гасим все светодиоды
  
  // Включаем светодиоды согласно таблице (кроме особых состояний)
  if(!isSpecialState(s)) {
    leds(stateTable[s].r, stateTable[s].y, stateTable[s].g);
  }
  
  // Выполняем действие при переходе
  stateTable[s].action();
}

// --- Обработка таймера состояния ---
void handleStateTimer() {
  if(st == RED_BLINK && blinkCnt <= 0) {
    next(GREEN);
    return;
  }
  
  if(isSpecialState(st)) {
    stateTable[st].update();
  } else {
    next(stateTable[st].nextState);
  }
}

// --- Инициализация ---
void setup(){
  pinMode(R,1); pinMode(Y,1); pinMode(G,1);
  pinMode(BTN,2); pinMode(EMERG,2);
  next(GREEN);
}

// --- Основной цикл ---
void loop(){
  // --- Обработка кнопки с антидребезгом ---
  bool p = !digitalRead(BTN);
  if(p && !btn && millis()-lastBtn > 50){ 
    btn = 1; 
    lastBtn = millis(); 
  }
  if(!p && btn){ 
    btn = 0; 
    unsigned long held = millis() - lastBtn;
    if(held > 2000) night = !night;
    else if(held > 50) ped = 1;
  }

  // --- Аварийный режим ---
  bool emerg = !digitalRead(EMERG);
  if(emerg){ 
    if(st != WARNING) next(WARNING, 500);
    if(millis() - t0 > dur) {
      stateTable[WARNING].update();
    }
    return;
  }
  
  if(!emerg && prevEmerg) next(GREEN);
  prevEmerg = emerg;

  // --- Ночной режим ---
  if(night){ 
    if(st != NIGHT) next(NIGHT, 1000);
    if(millis() - t0 > dur) {
      stateTable[NIGHT].update();
    }
    return;
  }

  // --- Обработка пешехода ---
  if(ped && st == GREEN){
    next(RED_BLINK, 500);
    ped = 0;
  }

  // --- Основной цикл светофора ---
  if(millis() - t0 > dur){
    handleStateTimer();
  }
}