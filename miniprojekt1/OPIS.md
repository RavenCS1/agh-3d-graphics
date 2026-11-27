# Mini-projekt 1 — Dron inspekcyjny w hangarze 3D

Dron startuje z lądowiska **H** na środku hangaru. Zadanie: podlecieć do każdego z trzech
stanowisk (**ALFA**, **BETA**, **GAMMA**) i przeprowadzić inspekcję. Misja kończy się, gdy
wszystkie trzy są sprawdzone; można ją zacząć od nowa bez zamykania programu.

## Sterowanie

| Klawisz | Działanie |
|---|---|
| `W` / `S` (lub `↑` / `↓`) | lot do przodu / do tyłu — względem nosa drona (niebieska kula z przodu) |
| `A` / `D` (lub `←` / `→`) | obrót drona w lewo / w prawo |
| `Q` / `E` | lot w bok (w lewo / w prawo) |
| `R` / `F` (lub `PgUp` / `PgDn`) | wznoszenie / opadanie |
| `Spacja` (trzymaj) | skanowanie stanowiska, w którego strefie jest dron |
| `Enter` / `Backspace` | restart misji (dron na lądowisko, wszystkie stanowiska niesprawdzone, stoper od zera) |
| `V` | kamera: pościg za dronem ↔ widok całego hangaru |
| LPM + ruch myszy, kółko | obrót i przybliżenie aktywnej kamery |
| `N` | reset kamer |
| `P` | pauza |
| `H` / `F1` | pokaż / ukryj panel sterowania |
| `F12` | zrzut ekranu do `MiniProjekt1_capture.png` |
| `Esc` | wyjście |

## Co zmieniłem w scenie startowej?

Kod podzieliłem na moduły:

- `mission.hpp/.cpp` — cała logika bez rysowania: stan drona (pozycja, prędkość, kurs),
  sterowanie, kolizje, inspekcja, zdarzenia, restart. Parametry rozgrywki są w `mp::tuning`.
- `hangar.hpp/.cpp` — scena startowa (podłoga, ściany, stanowiska, dron) plus nowe efekty.
- `hud_view.hpp/.cpp` — HUD 2D rysowany przez SFML na scenie OpenGL (`pushGLStates`/`popGLStates`).
- `main.cpp` — okno, pętla, dwie kamery orbitalne i obsługa klawiszy.
- Wspólne mechanizmy z laboratoriów są w `../g3d/` (kamera orbitalna, AABB, HUD, tekstury, zrzut ekranu).

Najważniejsze zmiany:

- **Sterowanie dronem** z prędkością zależną od `dt`: prędkość i obrót dążą płynnie do wartości
  zadanych (`smoothTowards`), więc ruch ma bezwładność i nie zależy od liczby klatek.
- **Kolizje** dron–stanowisko (AABB podstawy, kolumny i latarni) z „ślizganiem się" po przeszkodzie
  (`g3d::slideMove`) oraz ograniczenie lotu do wnętrza hangaru, nad podłogą i pod sufitem.
- **Kamera pościgowa** podążająca za dronem (mysz ustawia stały kąt względem jego nosa) i oryginalna
  kamera hangaru — przełączane klawiszem `V`.
- **HUD**: panel misji ze stoperem i stanem każdego stanowiska (odległość, postęp, czas inspekcji),
  minimapa z kursem drona i strefami, kompas do najbliższego niesprawdzonego stanowiska, podpisy
  stanowisk w 3D (`gluProject`), podpowiedź kontekstowa, komunikaty o zdarzeniach, panel sterowania,
  pauza i plansza końcowa. Stan misji jest też w tytule okna i w konsoli.
- Lądowisko **H** jako punkt startu/restartu.

## Jak działa inspekcja?

1. **Rozpoznanie stanowiska** — w każdej klatce `updateInspection()` liczy odległość drona od środka
   latarni każdego stanowiska (`beaconPosition(i)`, punkt `(x, 0.77, z)` z tablicy `STATIONS`).
   Stanowisko jest „w zasięgu", gdy odległość 3D jest mniejsza niż `SCAN_RADIUS = 1.1`. Strefę widać
   jako obracającą się siatkową kulę, gdy dron jest blisko, i jako okrąg na minimapie. Dzięki temu,
   że liczy się odległość 3D, nie da się zaliczyć stanowiska, przelatując wysoko nad nim.
2. **Warunek interakcji** — dron musi być w strefie i trzymać `Spację` przez `SCAN_TIME = 1.4 s`.
   Postęp (`scanProgress`) rośnie tylko w strefie; po puszczeniu Spacji powoli spada, a po wyjściu ze
   strefy lub przejściu do innego stanowiska zeruje się (postęp nie przechodzi na inne stanowisko).
3. **Zapamiętanie wyniku** — po osiągnięciu 100% ustawiane jest `inspected[i] = true` oraz czas
   inspekcji (`inspectedClock[i]`). Stanowisko sprawdzone nie jest skanowane drugi raz: w jego strefie
   postęp zawsze wynosi 0, a `Spacja` wyświetla komunikat „było już sprawdzone". `Spacja` poza strefą
   daje komunikat „za daleko".
4. **Koniec misji** — gdy `inspectedCount() == 3`, ustawiane jest `finished = true`, stoper się
   zatrzymuje, pojawia się plansza **MISJA ZAKOŃCZONA** z czasem, nad stanowiskami wyrastają zielone
   słupy światła, latarnie pulsują razem, a tło robi się lekko zielone.
5. **Restart** — `resetMission()` przypisuje świeży obiekt `Mission` i ustawia drona na lądowisku.

## Animacja lub efekt graficzny

- **Stan stanowiska w kolorze latarni** (z emisją materiału): niebieska — czeka, jaśniejsza i szybciej
  pulsująca — dron w zasięgu, migocząca od niebieskiej do bursztynowej — skanowanie (kolor zależy od
  postępu), zielona — sprawdzona. Na podłodze łuk postępu skanowania i podwójny zielony okrąg po inspekcji.
- **Promień skanera** — półprzezroczysty stożek od drona do latarni (wektory prostopadłe do kierunku
  promienia z iloczynu wektorowego), migoczący w czasie skanowania.
- **Fala** — dwa rozszerzające się i zanikające pierścienie po udanej inspekcji (1.4 s).
- **Dron**: pochyla się w kierunku lotu i przy skręcie (kąt zależy od prędkości w układzie drona),
  wirniki kręcą się szybciej przy większym ciągu (sąsiednie w przeciwne strony), tarcze wirników są
  półprzezroczyste, dron delikatnie unosi się w zawisie, ma migające światło na grzbiecie i na chwilę
  robi się czerwony po kolizji (razem z czerwoną ramką ekranu).
- **Miękki cień** pod dronem (wachlarz trójkątów z alfą malejącą do krawędzi) — rozmiar i krycie zależą
  od wysokości, co ułatwia ocenę położenia.
- **Zakończenie misji**: słupy światła, wspólne pulsowanie latarni, zielonkawe tło.

Obiekty półprzezroczyste są rysowane po nieprzezroczystych, z `GL_BLEND` i `glDepthMask(GL_FALSE)`
(test głębokości pozostaje włączony) — tak jak w zadaniu 16.

## Jak sprawdzić poprawność?

1. Uruchom program — panel pokazuje `0 / 3`, stoper rośnie, komunikat „Nowa misja".
2. Naciśnij `Spację` na lądowisku → komunikat „Za daleko" (nic się nie zalicza).
3. Leć do ALFA (np. `Q` ok. 1 s, potem `W` ok. 0.6 s). Po wejściu w strefę kula strefy robi się
   bursztynowa, a panel pokazuje „w zasięgu". Puść klawisze ruchu i przytrzymaj `Spację` ~1.5 s →
   pasek dochodzi do 100%, ALFA zielona, komunikat „Stanowisko ALFA sprawdzone (1/3)", fala.
4. Spróbuj ponownie `Spacją` przy ALFA → komunikat „było już sprawdzone", licznik dalej `1/3`.
5. Wleć w stanowisko → dron zatrzymuje się (nie przenika przez przeszkodę), komunikat „Kolizja".
6. Leć do BETA (`E` ok. 2 s) i GAMMA (`S` ok. 1.8 s), za każdym razem skanując `Spacją`.
   Po trzecim: plansza **MISJA ZAKOŃCZONA** z czasem, stoper stoi, słupy światła.
7. Naciśnij `Enter` → dron na lądowisku, `0 / 3`, stoper od zera, wszystkie stanowiska niebieskie.
8. Dodatkowo: `V` (zmiana kamery), `P` (pauza zatrzymuje stoper i animacje), zmiana rozmiaru okna
   (HUD dopasowuje się do nowego rozmiaru).

Scenariusz został sprawdzony automatycznie (Xvfb + xdotool): trzy inspekcje, próba ponownej
inspekcji, kolizje, plansza końcowa, pauza i restart.

## Znane ograniczenia

- Kolizje są liczone dla prostopadłościanu otaczającego drona (AABB niezależny od obrotu), więc przy
  ukośnym podejściu dron zatrzymuje się trochę dalej od przeszkody niż wynika z kształtu ramion.
- Latarnia stanowiska jest w kolizjach przybliżona sześcianem, a nie kulą.
- Ściany hangaru są tylko z dwóch stron; z pozostałych lot ogranicza niewidoczna granica hangaru.
- Napisy HUD wymagają czcionki `assets/fonts/DejaVuSans.ttf`. Bez niej HUD rysuje tylko kształty
  (paski, minimapę, kompas), a stan misji widać w tytule okna i w konsoli.
- Brak dźwięku i zapisu najlepszego czasu — naturalne rozszerzenia.
