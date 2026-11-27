# Laboratorium 5 — odpowiedzi na pytania z zadania 20

**1. Gdzie w wariancie CPU i GPU obliczana jest wysokość?**

- **CPU** (`C` → tryb CPU): w C++, w funkcji `deformOnCpu()` w `main.cpp`. W każdej klatce program
  liczy `y = -0.35 + A·sin(πx − phase)·cos(πz + phase)` dla wszystkich 441 wierzchołków, potem
  normalne trójkątów (iloczyn wektorowy) i uśrednione normalne wierzchołków. Gotowe dane idą do
  OpenGL przez `glVertex3f()` i `glNormal3f()`, a `wave.vert` dostaje `uGpuWave = 0` i nic nie zmienia.
- **GPU** (`C` → tryb GPU): w shaderze wierzchołków `shaders/wave.vert`. C++ wysyła płaską siatkę
  `y = -0.35` z normalną `(0, 1, 0)`, przygotowaną raz w `makeFlat()`. Wysokość i normalną dla
  każdego wierzchołka liczy karta graficzna przy każdym rysowaniu (`uGpuWave = 1`).

Oba warianty używają tego samego wzoru i tego samego czasu (`phase = 1.5 · wave.time`), dlatego
przełączenie `C` w trakcie animacji nie powoduje skoku fazy ani amplitudy.

**2. Dlaczego normalne muszą odpowiadać deformacji?**

Oświetlenie rozproszone (Lambert) zależy od kąta między normalną a kierunkiem do światła:
`0.25 + 0.75 · max(dot(N, L), 0)`. Jeśli przesuniemy wierzchołki, a zostawimy normalne płaskiej
siatki `(0, 1, 0)`, cała powierzchnia będzie oświetlona tak, jakby była płaska: kształt fal widać
tylko na krawędziach, a na zboczach nie ma jasnych i ciemnych stron. Normalna musi być prostopadła
do aktualnej powierzchni, czyli przy `y = h(x, z)` równa znormalizowanemu wektorowi
`(-dh/dx, 1, -dh/dz)`, a dla CPU — uśrednionym normalnym trójkątów po deformacji.
Można to sprawdzić, zmieniając amplitudę (`Z`/`X`): przy `A = 0` jasność jest jednolita, a przy
większej amplitudzie zbocza zwrócone do światła są wyraźnie jaśniejsze.

**3. Które parametry można aktualizować bez modyfikowania wierzchołków po stronie CPU?**

Wszystkie wartości przekazywane jako `uniform`: faza `uPhase` (czas animacji), amplituda
`uAmplitude`, częstotliwość `uFrequency` (rozszerzenie), przełącznik `uGpuWave`, położenie światła
`uLightWorld`, a w shaderze z zadania 19 także `uTime`, `uGrayscale`, `uPulse` i tekstura `uTexture`.
Zmiana uniformu to jedno wywołanie `setUniform()` na klatkę zamiast przeliczania i ponownego wysyłania
441 pozycji i normalnych. Bez zmian wierzchołków działają też kamera i projekcja (macierze
`gl_ModelViewMatrix`, `gl_ProjectionMatrix`, ustawiane przez `gluLookAt` / `gluPerspective`).

---

**Dodatkowo (zadanie 16): `glDepthMask(GL_FALSE)` a `glDisable(GL_DEPTH_TEST)`**

`glDepthMask(GL_FALSE)` wyłącza tylko *zapis* do bufora głębi — fragmenty warstwy nadal są
porównywane z głębokością powierzchni, więc fale wystające ponad warstwę ją zasłaniają.
`glDisable(GL_DEPTH_TEST)` wyłącza *porównywanie* — warstwa zostaje narysowana na wszystkim, co już
jest w buforze, także na falach, które powinny ją przesłaniać (w Lab 4 można to porównać klawiszem `U`).
