#include <WiFi.h>
#include <WebServer.h>
#include "LittleFS.h"
#include <ArduinoJson.h>

//

int current_millis;
static const BaseType_t app_cpu = 1;

static TaskHandle_t protocol_task = NULL;

int64_t position = 0;

//MOTOR pinek
#define IN1 19
#define IN2 18
#define ENA 5
//Enkóder pinek
#define ENC_A 21
#define ENC_B 33
//pinek
#define LED_PIN 23
#define ADC 36

#define TASKDELAY 500
//http
#define HTTP_REST_PORT 80
WebServer server(HTTP_REST_PORT); //Mi toszomat csinál ez itt, kérdheted. Nos amikor te, majd kapsz egy IP címet, amelyet majd ki fog írni neked az esp32 (mert megírtad ebben a programban) és azt te beírod a laptop böngészőjébe pl. http://192.168.114.42/ akkor azt automatikusan úgy kezeli mintha ezt írtad volna be http://192.168.114.42:80/. 
//Ez az alap port és ezért adunk itt meg neki 80-at. A port megadása fontos, hogy úgymond jó ajtón kopogj be, mert ha nem az ismerősöd ajtaján nyitsz be akkor azt fogják mondani, hogy takarodj és ugyanígy a hálózaton is! Ha itt mást adsz meg mint 80, akkor azt külön még hozzá kell tenned az url megadásakor a 80 helyére.

//ELŐSZÖR A void setup(){} RÉSZNÉL KEZD!! Így van felépítve a kommentelések. Meg ebben a sorrendben fut le a program is!

void handle_onConnect() { //Ha a böngésződben beíródott routertől kapott IP cím, akkor ez fog lefutni.
  File file_html = LittleFS.open("/index.html", "r"); //Kezdi azzal, hogy létrehoz egy File típusú, file_html névre hallgató változót, amely a fantasztikus LittleFS.open-el megynitja az esp32-re feltöltött index.html nevű fájlunkat és ebbe a változóba menti el. (Bármi más is lehet a html neve, csak a kiterjesztése legyen html!)

  if (!file_html) { //Ha ez nem jött össze valami csodával határos módon, akkor azt kijelezzük a felhasználó számára. Ugyanis a file_html-nek van egy bool visszatérésű válasza, ami akkor hamis ha valami nem jött össze.
    server.sendHeader("Access-Control-Allow-Origin", "*"); //Itt hozzáférést biztosítunk mindenkinek, aki szereti.
    server.send(200, "text/plain", "FAILED AT OPENING THE FILE");
    return;
  }

  server.sendHeader("Access-Control-Allow-Origin", "*"); //Itt hozzáférést biztosítunk mindenkinek, aki szereti, de tényleg. 
  server.streamFile(file_html, "text/html"); //Amenyiben sikerült megnyitni a fájlt, akkor ki küldi a WiFi-n keresztül a böngészőnek a file_html tartalmát. Itt a "text/html"-nél adjuk meg a kiküldött fájl kiterjesztését, valamint hogy szövegesen küldjük.
  file.close(); //Ha már megnyitottuk, akkor zárjuk is be. Ezért még a windows is szól amikor le akarod állítani, de vannak megnyitott fájlok. A fájl tartalma már el lett küldve a böngészőnek szóval nyugodtan be lehet zárni.
  //Jöhet a kövi függvény.
}

void handle_onledOn() { //Ez itt már komolyabb terep! Itt van minden is.
  //Ugyebár a http://192.168.114.42/ledOn url-n egy HTTP_POST módszer jelent meg, amelyet detektált a server.handleClient(); függvény és elindította ezt a függvényt.
  String postData = server.arg("plain"); //Ezen az url-n megjelent egy Json szöveg, amelyet most string-ként olvasunk be a postData változóba a server.arg() paranccsal, amelyben a "plain" azt jelöli, hogy nyersen olvassa ki amit talál. Jelen esetünkben valami ilyesmit olvas be: { "code": 107 } amely egy JSON formátumra utal.
  int success = 1; //mivel bejutottunk ebbe a függvénybe azt lehet mondani, hogy a HTTP_POST sikeres volt ezért ezt egy változóval is jelezzük.
  Serial.println("ledOn event"); //Kírjuk a soros porton, hogy megtörtént a HTTP_POST és elindult ez a függvény.

  StaticJsonDocument<50> jPostDoc; //Létrehozunk egy JSON típust váró változót, amely 50 byte adotot vár. A mi esetünkben pont elég ennyi hely, de ha ennél több elemet tartalmazó JSON-t küldünk ide, akkor 50 nem lesz elég. (Ezt az 50 byte helyet a stack-en készít el.)
  deserializeJson(jPostDoc, postData); //Fogja és átkonvertálja a string változónkat JSON formátumúvá. Ha nincs meg a JSON formátum, akkor nem lehet megállapítani az objektumait.
  JsonObject jPostObj = jPostDoc.as<JsonObject>(); //Itt létrehoz már egy JSON objektumot, amely már elemenkénti elérést eredményez. Azaz ezzel már lekérdezhetjük a JSON "code" elemének az értékét.

  if ((jPostObj.containsKey("code")) && (jPostObj["code"].as<int>() == 107) && (digitalRead(LED_PIN) == LOW)) { //Itt már egy ismerősebb szerkezetet láthatunk (Remélem, hogy tényleg ismerős! Ha nem akkor nem tudom mit keresel itt. Nem itt kell ezt elkezdeni!)
    //Megnézzük, hogy tartalmaz-e "code" elemet, vagyis kulcsot és, hogy ennek a "code" kulcsnak az értéke egyenlő-e 107-el és hogy a LED pinünk LOW-on van.
    digitalWrite(LED_PIN, HIGH); //Ha ez mind igaz, akkor felkapcsoljuk a LED-et.
    success = 1; //És a succes értékét 1-re állítjuk.
  } else if ((jPostObj.containsKey("code")) && (jPostObj["code"].as<int>() == 105) && (digitalRead(LED_PIN) == HIGH)) {
    //Itt annyi a különbség, hogyha a "code" értéke 105 és magasan van a LED pin, akkor kikapcsolja
    digitalWrite(LED_PIN, LOW);
    success = 1;
  } else
    //Amennyiben a "code" érték nem egyezik, vagy már eleve olyan állapotban volt a LED, amire kapcsolni szerettük volna, akkor a success 0 értéket vesz fel.
    success = 0;

  StaticJsonDocument<50> jsonBuffer; //Létrehozunk megint egy JSON documentumot.
  //Megint Objektummá avanzsáljuk. Vegyük észre, hogy itt .to van használva. Az előzőnél egy JSON dokument típust olvastunk be, amely már tartalmazott értéket ezért .as volt használva.
   JsonObject obj = jsonBuffer.to<JsonObject>(); //Itt még nincs semmi a JSON dokumentumunkban azaz a jsonBuffer-ban, ezért .to-val azt érjük el, hogy létrehozzuk ebbe a JSON objektumot.
   obj["success"] = success; //Itt megadjuk a JSON objektumunknak, hogy a "success" kulcsának az értéke legyen egyenlő a (int) success változónk értékével.
  char JSONMessageBuff[50]; //Létrehozunk egy char tömböt, ami lényegében egy string is lehetne, de így megadva fixálni tudjuk, hogy csak 50 byte, azaz 50 karakter tároljon.
  serializeJsonPretty(obj, JSONMessageBuff); //Ez az ellenkezője a deserializeJson-nak ugyanis ez JSON-ből csinál stringet (ami ugyebár egy char tömbként lehet értelmezni. Mi a szó (string)? Hát betűk (char) sorozata.)

  server.sendHeader("Access-Control-Allow-Origin", "*"); //Kinyitjuk a kapukat. (Jobb példa nem jutott az eszembe. Annyira még én sem értem ez pontosan hogyan működik, szóval lehet kicsit rossz a példa, de nagyából bemutatja).
  server.send(200, "application/json", JSONMessageBuff); //Itt elküldjük azt a char tömbünket, amibe a JSON-t írattuk bele. A 200-as az a html-k oké kódja (a jól ismert 404 is egy ilyen kód, de az messze van az okétól). Az "application/json" a böngésződ tudni fogja hogy json-t kap, szóval nem fogja azt hinni, hogy hülyeséget akarsz küldeni neki. Innen is köszönöm a böngésződ nevében.
}
//Figyelj az előző tartalmazott mindent ami ahhoz kell, hogy tudjál fogadni és küldeni a böngésző felé. Ami fontos! A HTTP metódust azaz a POST és GET-et a böngésződben futó html script fogja meghatározni. Ez a mikrovezérlőd számára annyiban fontos, hogy tudja melyik függvényt kell lefuttatnia.
//Persze ez nem azt jelenti, hogy akkor POST vagy GET-et csak úgy cserélgetheted, mert ezek leírják a böngésző számára, hogy most melyik folyamat fog lejátszódni, ami azért nagyon megegyszerűsíti a te életedet is.
//A metódusok a böngésződ oldaláról kell nézni, nem a mikrovezérlő oldaláról. Ha POST-ot csinálsz akkor a böngésződből megy ki adat a mikrovezérlő felé. Ha GET-et csinálsz, akkor meg valami választ vár a mikrovezérlőtől.
//Ami még fontos, hogy ezeket a folyamatokat csakis a böngésző oldalról tudod indítani. Tehát ha a böngésző nem ad ki POST-ot vagy GET-et, akkor hiába küldene bármit is a mikrovezérlő azt nem fogja fogadni a böngésző.
//Ha ezt így fel tudod fogni, akkor menni fog. Mostmár csak imádkozz a gépistenhez, hogy minden jól menjen!
void handle_onGetAdc() { 
  int adcVal = analogRead(ADC);
  Serial.println(adcVal);

  Serial.println("Megérkezett a jel");

  StaticJsonDocument<50> jsonBuffer;
  JsonObject obj = jsonBuffer.to<JsonObject>();
  obj["value"] = adcVal;
  char JSONmessageBuff[50];
  serializeJsonPretty(obj, JSONmessageBuff);

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", JSONmessageBuff);

  Serial.println("Sent: " + String(adcVal));
}

void doUserInteraction(void *parameters) { // A WiFi beállító
  WiFi.mode(WIFI_STA); //Itt az állítódik be, hogy milyen módban legyen az esp wifi modulja. az STA a station azaz állomás módra utal, azaz ő lesz akitől kapjuk a cuccot.
  WiFi.begin("ABB RobotStudio", "6XbAWHuGqR"); //A megannyi WiFi közül, amelyet érzékel a "levegőben" kiválasztjuk a mi általunk használtat és hogy értelmezni is tudjuk amit ettől a WiFi-től kapunk ahhoz meg kell adni a jelszót is.
  while (WiFi.status() != WL_CONNECTED) { //Visszajelzést ad addig amíg fel nem áll a WiFi kapcsolat.
    Serial.print(".");
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }

  //Itt kiíratjuk azt az IP címet, amelyet a WiFi router az ESP32-nek adományozott. Ezért örökké hálásak leszünk a routernek, hogy méltónak talált minkat arra, hogy az ő általa használt frekvencia tartományt használhatjuk.
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  //Itt azok kerülnek megadásra, hogy amikor a böngészőben adott url címre milyen HTTP módot hajtson végre. Esetünkben ez az url alapjáraton, valahogy így néz ki: http://192.168.114.42 (az IP cím eltérhet) ez a root elérés, azaz az a rész amikor még nem léptünk be semmilyen mappába pl mint ha a C meghajtóban lennénk.
  server.on("/", HTTP_GET, handle_onConnect); //Amikor beírod a böngészőben az IP címet akkor az egy HTTP_GET-nek felel meg és mivel az egy gyökér könyvtár elérés ezért kell itt "/" megadni. 
  server.on("/ledOn", HTTP_POST, handle_onledOn);
  server.on("/getAdc", HTTP_GET, handle_onGetAdc);
  server.begin(); //Ez elindítja a webservert. Innenstől fogva ha te beírod az IP címet a böngészőbe, amit a routertől kaptál, akkor azt megfogja kapni az esp32 is és ebben az esetben a handle_onConnect függéynt meghívja. Természetesen a készülék aminek a böngészőjébe beírod és az esp-nek is ugyan azon a WiFi-n kell lennie!

  while (1) { //Itt a handleClient függvény fut, ami arra jó, hogy figyeli, hogy érkezett-e bármilyen HTTP kérés és ha igen akkor a fentebb említett opciók közül, valamelyikre igaz lesz, akkor az ahhoz tartozó függvényt fogja végrehajtani.
    server.handleClient();
  }
  //Mostmár rátérhetsz a handle_onConnect függvényre.
}


void setup() {
  // Pinek beállítása. Ahogy nézem még teszünk arról is, hogy a LED pin biztosan ne legyen bekapcsolva. wow.
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.begin(115200); //Soros port bit-ráta beállítása. Fontos, hogy ha kiakarsz olvasni a soros porton, akkor ott is ez a szám legyen beállítva, így szonkronban lesznek.

  //Enkóder beolvasásra hasznos lesz. Bár lehet ezt is Task-hoz kéne rendelni?
  //attachInterrupt(digitalPinToInterrupt(ENC_A), encoderPos, RISING);

  if (!LittleFS.begin()) { //Ugyebár feltöltötted az esp23 memóriájába a data mappa tartalmát. Ez a parancs csak inicializálja/mountolja a mappa tartalmat azaz betölti a metaadatait és megnézi, hogy rendben van-e. Ha sikerült, akkor globálisan elérhetővé válnak a littleFS fájlműveletei.
    Serial.println("Mounting LittleFS FAILED"); //Értelem szerűen ha nem sikerült, akkor azt jelezzük a felhasználó felé.
    return;
  }

  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.printf("Serial peripheral started on core %d\n", xPortGetCoreID()); 

  xTaskCreatePinnedToCore(doUserInteraction, //Melyik függvény legyen a Task-hoz hozzáadva
                          "Do Interaction",
                          12288, //Stack méret byte-ban megadva. Ez van felhasználva a változók, különböző meghívott függvények és azok értékeinek tárolására meg ilyesmik. Valamint a POST és GET adatainak ideiglenes tárolására is használt.
                          NULL,
                          tskIDLE_PRIORITY, //Ez beállítja a legalacsonyabb prioritásra a task-ot azaz 0-ra. Erre ezt a szöveget kell beírni 0 helyett, ne kérdezd miért.
                          &protocol_task, //Nos ezt még én se tudtam igazán felfogni, de nem lényeges.
                          app_cpu); //melyik magon fusson.

  vTaskDelete(NULL); //Van ez a loopTask amely magában foglalja a setup egyszeri lefutását és a loop loopolását. Na ha ezt itt vagy a loop-ban beírod törli azt, azaz felszabadítja azt a kis időszeletet, amelyet elkért volna a loop üres futása. Ezt érdemes a setup végére rakni, hogy a setup le tudjon futni egyszer.
  //Nos itt már át is térhetsz a doUSerInteraction függvényre.
}

void loop() {
  // Üres mint a szegény ember pénztárcája.

}
