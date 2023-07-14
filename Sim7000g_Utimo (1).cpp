
//Define Serial1 Name
#define SIM7000 Serial1

//Define GSM Model
#define TINY_GSM_MODEM_SIM7000

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS


#include <TinyGsmClient.h>
#include <SPI.h>
#include <SD.h>
#include <Ticker.h>
#include <Firebase_ESP_Client.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SIM7000, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SIM7000);
#endif

// Configuración de la conexión a Internet
#define APN "bam.entelpcs.cl"  // APN de tu proveedor de red móvil
#define USERNAME "entelpcs"    // Nombre de usuario para la conexión (si es necesario)
#define PASSWORD "entelpcs"    // Contraseña para la conexión (si es necesario)

// Insert Firebase project API Key
#define API_KEY "AIzaSyDuCyn1GH6R4cl8ESlxvnamBv5oZKn26mg"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://petappdb-d8663-default-rtdb.firebaseio.com/"



#define UART_BAUD 9600
#define PIN_DTR 25
#define PIN_TX 27
#define PIN_RX 26
#define PWR_PIN 4

#define SD_MISO 2
#define SD_MOSI 15
#define SD_SCLK 14
#define SD_CS 13
#define LED_PIN 12cv

//Variables
int gnss_run_status = 0;
int fix_status = 0;
int year = 0;
int month = 0;
int day = 0;
int hour = 0;
int minutes = 0;
float secondWithSS = 0;
float lat = 0;
float lon = 0;
float msl_alt = 0;
float speed_over_ground = 0;
float course_over_ground = 0;
bool reserved1 = 0;
int fix_mode = 0;
int hdop = 0;
int pdop = 0;
int vdop = 0;
bool reserved2 = 0;
int gnss_satellites_used = 0;
int gps_satellites_used = 0;
int glonass_satellites_used = 0;
bool reserved3 = 0;
int c_n0_max = 0;
float hpa = 0;
float vpa = 0;
String historial[500];
String auxValue[500];

#define WIFI_SSID "XiaomiPoco"
#define WIFI_PASSWORD "19841640"

// Define Firebase Data object.
FirebaseData fbdo;

// Define firebase authentication.
FirebaseAuth auth;

// Definee firebase configuration.
FirebaseConfig config;

// Boolean variable for sign in status.
bool signupOK = false;


void enableGPS(void) {
  // Set SIM7000G GPIO4 LOW ,turn on GPS power
  // CMD:AT+SGPIO=0,4,1,1
  // Only in version 20200415 is there a function to control GPS power
  modem.sendAT("+SGPIO=0,4,1,1");
  if (modem.waitResponse(10000L) != 1) {
    DBG(" SGPIO=0,4,1,1 false ");
  }
  modem.enableGPS();
}

void disableGPS(void) {
  // Set SIM7000G GPIO4 LOW ,turn off GPS power
  // CMD:AT+SGPIO=0,4,1,0
  // Only in version 20200415 is there a function to control GPS power
  modem.sendAT("+SGPIO=0,4,1,0");
  if (modem.waitResponse(10000L) != 1) {
    DBG(" SGPIO=0,4,1,0 false ");
  }
  modem.disableGPS();
}



void modemPowerOn() {
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(1000);  //Datasheet Ton mintues = 1S
  digitalWrite(PWR_PIN, HIGH);
}

void modemPowerOff() {
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(1500);  //Datasheet Ton mintues = 1.2S
  digitalWrite(PWR_PIN, HIGH);
}


void modemRestart() {
  modemPowerOff();
  delay(1000);
  modemPowerOn();
}

void setup() {
  // Set console baud rate
  Serial.begin(115200);

  delay(1000);

  modemPowerOn();

  SIM7000.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

  Serial.println("/**********************************************************/");
  Serial.println("To initialize the network test, please make sure your GPS");
  Serial.println("antenna has been connected to the GPS port on the board.");
  Serial.println("/**********************************************************/\n\n");

  delay(10000);

  if (!modem.testAT()) {
    Serial.println("Failed to restart modem, attempting to continue without restarting");
    modemRestart();
    return;
  }

  // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  Serial.println("Start positioning . Make sure to locate outdoors.");
  Serial.println("The blue indicator light flashes to indicate positioning.");

  enableGPS();

  // Assign the api key (required).
  config.api_key = API_KEY;

  // Assign the RTDB URL (required).
  config.database_url = DATABASE_URL;

  Serial.println("---------------Sign up");
  Serial.println("Sign up new user... ");
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  Serial.println("---------------");

  Firebase.begin(&config, &auth);
}
struct Historial {
    float latitud;
    float longitud;
    int year;
    int month;
    int day;
    int hour;
    int minutes;
    float seconds;
};


void loop() {
  String gps_raw = modem.getGPSraw();

  if (gps_raw != "") {
    gnss_run_status = splitter(gps_raw, ',', 0).toInt();
    fix_status = splitter(gps_raw, ',', 1).toInt();
    String date = splitter(gps_raw, ',', 2);  //yyyyMMddhhmm ss.sss
    year = date.substring(0, 4).toInt();
    month = date.substring(4, 6).toInt();
    day = date.substring(6, 8).toInt();
    hour = date.substring(8, 10).toInt();
    minutes = date.substring(10, 12).toInt();
    secondWithSS = date.substring(12, 18).toFloat();
    lat = splitter(gps_raw, ',', 3).toFloat();                 //±dd.dddddd
    lon = splitter(gps_raw, ',', 4).toFloat();                 //±ddd.dddddd
    msl_alt = splitter(gps_raw, ',', 5).toFloat();             //meters
    speed_over_ground = splitter(gps_raw, ',', 6).toFloat();   //Km/hour
    course_over_ground = splitter(gps_raw, ',', 7).toFloat();  //degrees
    fix_mode = splitter(gps_raw, ',', 8).toInt();
    reserved1 = splitter(gps_raw, ',', 9).toInt();
    hdop = splitter(gps_raw, ',', 10).toInt();
    pdop = splitter(gps_raw, ',', 11).toInt();
    vdop = splitter(gps_raw, ',', 12).toInt();
    reserved2 = splitter(gps_raw, ',', 13).toInt();
    gnss_satellites_used = splitter(gps_raw, ',', 14).toInt();
    gps_satellites_used = splitter(gps_raw, ',', 15).toInt();
    glonass_satellites_used = splitter(gps_raw, ',', 16).toInt();
    reserved3 = splitter(gps_raw, ',', 17).toInt();
    c_n0_max = splitter(gps_raw, ',', 18).toInt();  //dBHz
    hpa = splitter(gps_raw, ',', 19).toFloat();     //meters
    vpa = splitter(gps_raw, ',', 20).toFloat();     //meters

    Serial.println("----------------------------------");

    Serial.print("GNSS Run Status:");
    Serial.println(gnss_run_status);
    Serial.print("Fix Status:");
    Serial.println(gnss_run_status);
    Serial.print("date:");
    Serial.println(date);
    Serial.print("Latitude:");
    Serial.println(lat, 6);
    Serial.print("Longitude:");
    Serial.println(lon, 6);
    if (lat!=0 && lon != 0){

      /*Historial historialArray[1];
      historialArray[0].latitud = lat;
      historialArray[0].longitud = lon;
      historialArray[0].year = year;
      historialArray[0].month = month;
      historialArray[0].day = day;
      historialArray[0].hour = hour;
      historialArray[0].minutes = minutes;
      historialArray[0].seconds = secondWithSS;
      std::string historialString;
      historialString += std::to_string(historialArray[0].latitud) + "-"
            + std::to_string(historialArray[0].longitud) + "-"
            + std::to_string(historialArray[0].year) + "-"
            + std::to_string(historialArray[0].month) + "-"
            + std::to_string(historialArray[0].day) + "-"
            + std::to_string(historialArray[0].hour) + "-"
            + std::to_string(historialArray[0].minutes) + "-"
            + std::to_string(historialArray[0].seconds) + "; ";
      historial.push(historialString);
      */
      enviarDatosFirebase(lat, lon, date);
    }
    delay(2000);
  }
}


void enviarDatosFirebase(float latitud, float longitud, String historial) {
  // Write an Int number on the database path test/random_Float_Val.
  if (Firebase.RTDB.setFloat(&fbdo, "Ubicacion/Latitud", latitud)) {
    Serial.println("PASSED");
    Serial.println("PATH: " + fbdo.dataPath());
    Serial.println("TYPE: " + fbdo.dataType());
  } else {
    Serial.println("FAILED");
    Serial.println("REASON: " + fbdo.errorReason());
  }
  // Write an Float number on the database path test/random_Int_Val.
  if (Firebase.RTDB.setInt(&fbdo, "Ubicacion/Longitud", longitud)) {
    Serial.println("PASSED");
    Serial.println("PATH: " + fbdo.dataPath());
    Serial.println("TYPE: " + fbdo.dataType());
  } else {
    Serial.println("FAILED");
    Serial.println("REASON: " + fbdo.errorReason());
  }
  if (Firebase.RTDB.setTimestamp(&fbdo, "Ubicacion/Historial", date)) {
    Serial.println("PASSED");
    Serial.println("PATH: " + fbdo.dataPath());
    Serial.println("TYPE: " + fbdo.dataType());
  } else {
    Serial.println("FAILED");
    Serial.println("REASON: " + fbdo.errorReason());
  }
}
String splitter(String data, char separator, int index) {
  int stringData = 0;
  String dataPart = "";

  for (int i = 0; i < data.length(); i++) {

    if (data[i] == separator) {

      stringData++;
    } else if (stringData == index) {

      dataPart.concat(data[i]);
    } else if (stringData > index) {

      return dataPart;
      break;
    }
  }

  return dataPart;
}