// --- Пины светодиодов и кнопок ---
const int R=13,Y=12,G=11,BTN=2,EMERG=3;

// --- Состояния светофора ---
enum St{GREEN,YELLOW,RED,RED_BLINK,WARNING,NIGHT}; 
St st; // текущее состояние

// --- Переменные для таймеров и состояний ---
unsigned long t0,dur,lastBtn=0; // t0 — момент начала состояния, dur — длительность, lastBtn — время последнего нажатия кнопки
bool ped=0,night=0,btn=0,prevEmerg=0; // ped — запрос пешехода, night — ночной режим, btn — флаг удержания кнопки, prevEmerg — предыдущее состояние аварийки
int blinkCnt=0; // счётчик миганий красного при коротком нажатии

// --- Функция управления светодиодами ---
void leds(int r,int y,int g){
  digitalWrite(R,r);
  digitalWrite(Y,y);
  digitalWrite(G,g);
}

// --- Переход в новое состояние ---
void next(St s,unsigned long d){
  st=s;               // устанавливаем текущее состояние
  t0=millis();        // сохраняем время начала состояния
  dur=d;              // сохраняем длительность
  leds(0,0,0);        // гасим все светодиоды перед включением нужного
  switch(s){
    case GREEN: leds(0,0,1); break;       // включаем зелёный
    case YELLOW: leds(0,1,0); break;      // включаем жёлтый
    case RED: leds(1,0,0); break;         // включаем красный
    case RED_BLINK: blinkCnt=6; break;    // для короткого нажатия: 3 мигания по 500 мс
  }
}

// --- Инициализация ---
void setup(){
  pinMode(R,1); pinMode(Y,1); pinMode(G,1);     // настраиваем светодиоды как выходы
  pinMode(BTN,2); pinMode(EMERG,2);            // настраиваем кнопки как входы с pull-up
  next(GREEN,10000);                            // стартуем с зелёного на 10 секунд
}

// --- Основной цикл ---
void loop(){
  // --- Обработка кнопки с антидребезгом ---
  bool p=!digitalRead(BTN);                        // LOW → кнопка нажата
  if(p&&!btn && millis()-lastBtn>50){ btn=1; lastBtn=millis(); } // фиксация начала нажатия
  if(!p&&btn){                                     // кнопка отпущена
    btn=0; 
    unsigned long held=millis()-lastBtn;          // сколько держали кнопку
    if(held>2000) night=!night;                   // длинное нажатие → включаем/выключаем ночной режим
    else if(held>50) ped=1;                       // короткое нажатие → запрос пешехода
  }

  // --- Аварийный режим ---
  bool emerg=!digitalRead(EMERG);                 // проверяем состояние аварийки
  if(emerg){ 
    if(st!=WARNING) next(WARNING,500);           // переход в WARNING
    if(millis()-t0>dur){                         // мигание жёлтым
      digitalWrite(Y,!digitalRead(Y));
      t0=millis();
    }
    return;                                      // аварийка имеет приоритет
  }
  if(!emerg && prevEmerg) next(GREEN,10000);     // аварийка выключена → возвращаемся к циклу
  prevEmerg=emerg;

  // --- Ночной режим ---
  if(night){ 
    if(st!=NIGHT) next(NIGHT,1000);              // переключаемся в ночной режим
    if(millis()-t0>dur){                         // мигание жёлтым с периодом 1 с
      digitalWrite(Y,!digitalRead(Y));
      t0=millis();
    }
    return;                                      // ночной режим имеет приоритет
  }

  // --- Обработка пешехода (короткое нажатие) ---
  if(ped && st==GREEN){                           // короткое нажатие на зелёный → мигает красный
    next(RED_BLINK,500);
    ped=0;                                        // сброс запроса пешехода
  }

  // --- Основной цикл светофора ---
  if(millis()-t0>dur) switch(st){
    case GREEN:  next(YELLOW,3000); break;                     // после зелёного → жёлтый
    case YELLOW: next(RED,10000); break;                       // после жёлтого → красный
    case RED:    next(GREEN,10000); break;                     // после красного → зелёный
    case RED_BLINK:                                              
      digitalWrite(R,!digitalRead(R));                         // мигание красного
      blinkCnt--; 
      t0=millis();                                              // сброс таймера для мигания
      if(blinkCnt<=0) next(GREEN,10000);                       // после миганий → зелёный
      break;
  }
}