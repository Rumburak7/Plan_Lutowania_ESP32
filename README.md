
# Plan Lutowania (ESP32 + karta SD)

Własne narzędzie do rysowania schematów połączeń pinów (np. ESP32 → TFT), drukowania ich z listą przewodów i lutowania według wydruku. Cały program to **jeden plik `Plan_Lutowania_ESP32.ino`**: ESP32 wystawia stronę WWW, a schematy zapisuje na karcie SD.

Używa tylko bibliotek dołączonych do pakietu ESP32 (`WiFi`, `WebServer`, `SPI`, `SD`, `ESPmDNS`), więc nie trzeba niczego doinstalowywać.

---

## Co jest potrzebne

- płytka ESP32 (klasyczna albo S3; przy S3 zobacz uwagę o pinach niżej),
- moduł karty SD (SPI) z kartą sformatowaną jako FAT32,
- sieć WiFi (albo telefon/komputer podłączony do sieci, którą płytka utworzy sama),
- Arduino IDE z zainstalowanym pakietem płytek ESP32.

## Instalacja

1. Otwórz `Plan_Lutowania_ESP32.ino` w Arduino IDE.
2. Na górze pliku wpisz swoją sieć:

   ```cpp
   const char* WIFI_SSID = "TWOJA_SIEC";
   const char* WIFI_PASS = "TWOJE_HASLO";
   ```

3. Sprawdź piny karty SD (domyślnie takie jak w projektach „Magazyn części" i „Moje programy"):

   | Sygnał karty | Pin ESP32 |
   |---|---|
   | CS   | 21 |
   | SCK  | 36 |
   | MOSI | 35 |
   | MISO | 37 |

   Zasilanie modułu SD zgodnie z jego opisem (najczęściej 5 V albo 3,3 V) i wspólna masa z ESP32. Jeśli masz inne połączenie, zmień cztery linie `#define SD_...` na początku pliku.

4. Wybierz swoją płytkę, wgraj program i otwórz Monitor portu (115200).
5. W Monitorze zobaczysz adres strony, na przykład `http://192.168.1.50`. Wpisz go w przeglądarce. Często działa też `http://plan.local`.

Jeśli płytka nie połączy się z WiFi w ciągu 20 sekund, uruchomi własną sieć **`PlanLutowania`** (hasło **`lutowanie`**). Połącz się z nią i otwórz `http://192.168.4.1`.

> **Uwaga o pinach na ESP32-S3.** Na modułach z pamięcią PSRAM (N8R8, N16R8) piny GPIO35–37 są zajęte przez PSRAM, więc domyślne piny SD z powyższej tabeli nie zadziałają. Wybierz wtedy inne piny i zmień je w `#define`.

## Jak używać

### Rysowanie schematu

- **Dodaj element:** kliknij go na liście po lewej. Element ląduje obok poprzednich.
- **Poprowadź przewód:** przeciągnij od kropki jednego pinu do kropki drugiego. Można też kliknąć jeden pin, a potem drugi.
- **Kolor przewodu** dobiera się sam: masa (GND) jest czarna, zasilanie czerwone, a sygnały dostają kolejne kolory. Zaznacz przewód, żeby zmienić kolor ręcznie.
- **Numery przewodów** (pole „Numery" na górnym pasku) pojawiają się przy pinie docelowym i odpowiadają liście połączeń.
- **Styl przewodów:** Kątowa, Krzywa albo Prosta (przełącznik na górnym pasku). Przy stylu kątowym zaznaczony przewód ma uchwyt, którym przesuwasz jego pionowy odcinek.
- **Przesuwanie elementów:** przeciągnij element za nagłówek. Tło przeciągasz, żeby przesuwać widok, a kółko myszy przybliża i oddala.
- **Ostrzeżenia:** program zgłasza bezpośrednie połączenie masy z zasilaniem (zwarcie) oraz połączenie 3V3 z 5V.

### Obracanie i odwracanie

- **Obróć** (przycisk nad zaznaczonym elementem albo w panelu po prawej): obraca element o 90°. Piny lądują wtedy na górnej i dolnej krawędzi. Klawisz **R** obraca w prawo, **Shift+R** w lewo.
- **Odwróć strony:** zamienia lewą i prawą kolumnę pinów miejscami.
- **Widok od spodu** (górny pasek): odbija cały schemat lustrzanie, tak jak wygląda po przekręceniu płytek na drugą stronę. Przydaje się, gdy lutujesz od spodu. Arkusz robi się żółtawy, a wydruk ma dopisek „WIDOK OD SPODU". Ustawienie zapisuje się razem ze schematem.

### Własne elementy

Na dole listy jest przycisk **Własny element**. Wpisujesz nazwę oraz piny lewej i prawej kolumny, po jednym w linii. Nazwa alternatywna pinu po kresce pionowej, np. `GPIO23|MOSI`. Piny można później poprawić przyciskiem **Edytuj piny** na zaznaczonym elemencie. Element możesz też **Duplikować** albo **Usunąć**.

### Lutowanie z wydruku

- Przycisk **Wydruk** otwiera arkusz A4 (poziomo albo pionowo) ze schematem i listą połączeń z polami do odhaczania.
- Z arkusza można też pobrać **PNG**, **SVG** i **CSV** (rozdzielany średnikiem, otwiera się w Excelu).
- W zakładce **Lista** po prawej odhaczasz przewody, które już zlutowałeś. Pasek postępu pokazuje ile zostało.

### Projekty

Przycisk **Projekty** na górnym pasku:

- nowy pusty schemat,
- przykład (ESP32 + TFT),
- duplikowanie bieżącego schematu,
- lista zapisanych schematów (wczytanie i usuwanie, usuwanie wymaga drugiego kliknięcia),
- **Kopia projektu (JSON)**: pokazuje cały schemat jako tekst, który można skopiować, zapisać do pliku albo wkleić i wczytać z powrotem jako nowy projekt.

Zmiany zapisują się same po chwili. Status jest widoczny w prawym dolnym rogu okna.

## Skróty klawiszowe

| Klawisz | Działanie |
|---|---|
| `Ctrl+Z` | cofnij |
| `Ctrl+Shift+Z` lub `Ctrl+Y` | ponów |
| `Ctrl+D` | duplikuj zaznaczony element |
| `Delete` / `Backspace` | usuń zaznaczone |
| `R` / `Shift+R` | obróć element w prawo / w lewo |
| Strzałki | przesuń element o 10 (z `Shift` o 50) |
| `Esc` | anuluj / odznacz |

## Biblioteka elementów

- **Płytki MCU:** ESP32 DevKit V1 (30), ESP32 DevKitC (38), ESP32-S3 Mini USB-C, ESP32 Mini (D1 mini), ESP32-S3 DevKitC-1, NodeMCU ESP8266, Wemos D1 mini, Arduino Nano.
- **Wyświetlacze:** TFT SPI z dotykiem (14 pinów), TFT SPI (9), TFT ST7789, GC9A01, OLED SSD1306, MAX7219, panel RGB HUB75.
- **Czujniki:** MPU6050, BMP280, DS18B20, DHT11/DHT22, GUVA-S12SD, INA219, HC-SR501.
- **Moduły:** czytnik kart SD, GNSS/GPS, przekaźniki 4×, WS2812B, TP4056, ogniwo 18650, zasilacz 5 V.
- **Podstawowe:** dioda LED, rezystor, kondensator, przycisk, buzzer, szyny GND / 3V3 / 5V.

Opisy pinów na płytkach zostały wpisane na podstawie zdjęć i kart katalogowych. Przed lutowaniem porównaj kilka pinów (zasilanie i masę) z tym, co masz na swojej płytce, bo egzemplarze od różnych producentów bywają różne.

## Gdzie są dane

Na karcie SD, w folderze `/plan`:

- `index.json` to lista schematów,
- `p_<id>.json` to jeden schemat.

Pliki są zwykłym tekstem JSON, więc można je kopiować na komputer jako kopię zapasową. Program w ESP32 nie interpretuje ich zawartości, tylko je zapisuje i odczytuje.

## Interfejs HTTP (dla ciekawych)

| Adres | Metoda | Działanie |
|---|---|---|
| `/` | GET | strona programu |
| `/api/status` | GET | `ok` albo `nosd` (brak karty) |
| `/api/file?n=<nazwa>` | GET | odczyt pliku `/plan/<nazwa>.json` |
| `/api/file?n=<nazwa>` | POST | zapis (nadpisanie) pliku |
| `/api/del?n=<nazwa>` | POST | usunięcie pliku |

Nazwa może mieć do 40 znaków: litery, cyfry, `_` i `-`.

## Rozwiązywanie problemów

- **W Monitorze „Karta SD: BLAD":** sprawdź piny SD, zasilanie modułu i format karty (FAT32). Strona się otworzy, ale zapis nie zadziała.
- **Brak adresu w Monitorze:** źle wpisana nazwa lub hasło WiFi. Po 20 sekundach płytka utworzy sieć `PlanLutowania`.
- **`plan.local` się nie otwiera:** użyj adresu IP z Monitora. Nazwa `.local` bywa niedostępna na niektórych systemach i routerach.
- **Status „Brak połączenia z ESP32":** odśwież stronę i sprawdź, czy płytka jest zasilana i w tej samej sieci.
- **Nie otwiera się okno drukowania:** pobierz PNG albo SVG z okna „Wydruk" i wydrukuj je zwykłym sposobem.

## Bezpieczeństwo

Strona nie ma hasła i działa po `http`. Każdy, kto jest w tej samej sieci WiFi, może ją otworzyć i zmienić schematy. Używaj jej w domowej sieci, nie wystawiaj do internetu.

## Wersja w Claude

Ten sam program w wersji na stronę w Claude ma te same funkcje, ale zapisuje schematy w przeglądarce (i w koncie), a nie na karcie SD.
