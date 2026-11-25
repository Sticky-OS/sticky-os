// Tutor's lessons in Romanian: one LessonText a lesson, in slot order (0 is 7.2, the Pythagorean theorem;
// 1 to 8 are Unit 4's lessons 4.1 to 4.8). Included by tutor7_text.cpp. The plan of each lesson (its
// pages' figures, how many pages, its exercises) is in tutor7_lessons.cpp and is the same in every
// language: a language has exactly that many summary and example pages per lesson.
//
// A page: {"title", "body"}. In the body \n starts a paragraph; a square root is √ followed by its
// radicand (digits or letters) or a parenthesised expression (the screen draws the bar). A body
// stays under 620 bytes, so it fits a screen at 9 pt in portrait.
    // slot 0: 7.2
    {"Teorema lui Pitagora",
     "Geometrie",
     {
         {"Triunghiul dreptunghic",
          "Un triunghi dreptunghic are un unghi drept, de 90°.\n"
          "Latura opusă unghiului drept se numește ipotenuză și este cea mai lungă latură a triunghiului. Celelalte două laturi se numesc catete.\n"
          "În triunghiul ABC din figură, unghiul A este drept: BC este ipotenuza, iar AB și AC sunt catetele."},
         {"Teorema lui Pitagora",
          "Într-un triunghi dreptunghic, pătratul ipotenuzei este egal cu suma pătratelor catetelor.\n"
          "Pentru triunghiul ABC, cu A = 90°:\n"
          "BC² = AB² + AC²\n"
          "De aici putem afla orice latură: BC = √(AB² + AC²), AB = √(BC² − AC²), AC = √(BC² − AB²)."},
         {"Cum folosim teorema",
          "1. Găsește unghiul drept și spune care este ipotenuza.\n"
          "2. Scrie relația lui Pitagora pentru triunghiul respectiv.\n"
          "3. Înlocuiește laturile cunoscute și calculează.\n"
          "4. Extrage rădăcina pătrată și scoate factorii de sub radical, dacă se poate: √32 = √(16·2) = 4√2.\n"
          "Ipotenuza este mereu mai lungă decât oricare dintre catete."},
         {"Reciproca teoremei",
          "Dacă într-un triunghi suma pătratelor a două laturi este egală cu pătratul celei de-a treia laturi, atunci triunghiul este dreptunghic.\n"
          "Unghiul drept este opus celei mai lungi laturi. De aceea, la verificare, luăm cea mai lungă latură drept ipotenuză.\n"
          "Atenție: toate laturile trebuie să fie exprimate în aceeași unitate de măsură."},
         {"Cazuri speciale",
          "Triunghiul dreptunghic isoscel, cu catetele egale cu c, are ipotenuza c√2. De aceea diagonala unui pătrat cu latura l este l√2.\n"
          "Triunghiul echilateral cu latura l are înălțimea l√3/2: înălțimea îl împarte în două triunghiuri dreptunghice, cu ipotenuza l și o catetă l/2."},
     },
     {
         {"Exemplu: aflăm ipotenuza",
          "În triunghiul dreptunghic ABC, cu A = 90°, avem AB = 9 cm și AC = 40 cm. Cât este BC?\n"
          "Ipotenuza este BC, deci:\n"
          "BC² = AB² + AC² = 9² + 40² = 81 + 1600 = 1681\n"
          "BC = √1681 = 41 cm."},
         {"Exemplu: aflăm o catetă",
          "În triunghiul dreptunghic ABC, cu A = 90°, avem BC = 9 cm și AB = 7 cm. Cât este AC?\n"
          "Ipotenuza este BC, deci:\n"
          "AC² = BC² − AB² = 9² − 7² = 81 − 49 = 32\n"
          "AC = √32 = √(16·2) = 4√2 cm."},
         {"Exemplu: este triunghiul dreptunghic?",
          "Un triunghi are laturile de 20 cm, 21 cm și 29 cm. Este dreptunghic?\n"
          "Cea mai lungă latură este 29 cm, deci o încercăm drept ipotenuză:\n"
          "20² + 21² = 400 + 441 = 841, iar 29² = 841.\n"
          "Valorile sunt egale, deci triunghiul este dreptunghic, cu unghiul drept opus laturii de 29 cm."},
     }},
    // slot 1: 4.1
    {"Patrulaterul convex",
     "Geometrie",
     {
         {"Patrulaterul",
          "Un patrulater ABCD are patru vârfuri (A, B, C, D) și patru laturi: AB, BC, CD, DA. Laturile fără vârf comun sunt opuse, iar cele cu un vârf comun sunt consecutive.\n"
          "Diagonalele unesc vârfurile opuse: AC și BD.\n"
          "Îl putem citi pornind de la orice vârf, în orice sens (ABCD, BCDA, ADCB), dar nu ACBD."},
         {"Convex sau concav",
          "Un patrulater este convex dacă, pentru fiecare latură, celelalte două vârfuri sunt de aceeași parte a dreptei care conține latura. Altfel este concav.\n"
          "Într-un patrulater convex, segmentul care unește două puncte din interior rămâne în interior.\n"
          "Fiecare unghi al unui patrulater convex este mai mic de 180°."},
         {"Suma unghiurilor",
          "Teoremă: într-un patrulater convex suma măsurilor unghiurilor este 360°.\n"
          "Demonstrație: diagonala BD împarte patrulaterul în două triunghiuri, fiecare cu suma unghiurilor 180°. Deci A + B + C + D = 180° + 180° = 360°."},
         {"Cum folosim teorema",
          "Dacă știm trei unghiuri, al patrulea este 360° minus suma lor.\n"
          "Dacă unghiurile sunt direct proporționale cu niște numere, împărțim 360° la suma numerelor și aflăm cât valorează o parte. Fiecare unghi are atâtea părți câte indică numărul lui.\n"
          "Atenție: nu folosi 180°, aceasta este suma de la triunghi."},
     },
     {
         {"Exemplu: al patrulea unghi",
          "Un patrulater convex ABCD are unghiurile A = 70°, B = 100° și C = 80°. Cât măsoară D?\n"
          "A + B + C = 70° + 100° + 80° = 250°.\n"
          "D = 360° − 250° = 110°."},
         {"Exemplu: unghiuri proporționale",
          "Unghiurile unui patrulater convex sunt direct proporționale cu 8, 9, 11 și 12. Cât măsoară ele?\n"
          "Suma numerelor: 8 + 9 + 11 + 12 = 40, deci o parte are 360° : 40 = 9°.\n"
          "Unghiurile: 72°, 81°, 99° și 108°."},
         {"Exemplu: verificăm suma",
          "Un patrulater are unghiurile de 60°, 105°, 90° și 105°. Poate fi convex?\n"
          "Suma lor: 60° + 105° + 90° + 105° = 360°, iar fiecare unghi este mai mic de 180°.\n"
          "Da, suma este cea potrivită pentru un patrulater convex."},
     }},
    // slot 2: 4.2
    {"Paralelogramul",
     "Geometrie",
     {
         {"Definiție",
          "Paralelogramul este un patrulater cu laturile opuse paralele două câte două: AB și CD sunt paralele, iar BC și AD sunt paralele.\n"
          "Orice paralelogram este un patrulater convex."},
         {"Unghiurile",
          "Teoremă: într-un paralelogram a) unghiurile consecutive sunt suplementare, adică A + B = 180° (la fel B + C, C + D, D + A); b) unghiurile opuse sunt congruente: A = C și B = D.\n"
          "Reciproc: un patrulater convex cu unghiurile consecutive suplementare, sau cu unghiurile opuse congruente două câte două, este paralelogram."},
         {"Laturile",
          "Teoremă: într-un paralelogram laturile opuse sunt congruente: AB = CD și AD = BC. Perimetrul este P = 2 · (AB + BC).\n"
          "Reciproc: un patrulater convex cu laturile opuse congruente două câte două este paralelogram.\n"
          "Un patrulater convex cu două laturi opuse paralele și congruente este și el paralelogram."},
         {"Diagonalele",
          "Teoremă: într-un paralelogram diagonalele se înjumătățesc. Dacă ele se intersectează în O, atunci AO = OC și BO = OD.\n"
          "Reciproc: un patrulater convex ale cărui diagonale se înjumătățesc este paralelogram."},
         {"Cum îl recunoaștem",
          "Un patrulater convex este paralelogram dacă are: laturile opuse paralele; laturile opuse congruente; unghiurile consecutive suplementare; unghiurile opuse congruente; diagonalele care se înjumătățesc; sau două laturi opuse paralele și congruente.\n"
          "Atenție: doar două laturi opuse paralele, sau doar două laturi opuse congruente, nu ajung."},
     },
     {
         {"Exemplu: unghiurile",
          "Paralelogramul ABCD are unghiul A = 70°. Cât măsoară B, C și D?\n"
          "B este consecutiv cu A: B = 180° − 70° = 110°.\n"
          "C este opus lui A: C = 70°.\n"
          "D este opus lui B: D = 110°."},
         {"Exemplu: diagonalele",
          "Diagonalele paralelogramului ABCD se intersectează în O. Se știe că AO = 3 cm și DO = 5 cm. Cât sunt CO și BD?\n"
          "Diagonalele se înjumătățesc: CO = AO = 3 cm.\n"
          "BO = DO = 5 cm, deci BD = 2 · 5 = 10 cm."},
         {"Exemplu: perimetrul",
          "Un paralelogram are perimetrul de 30 cm, iar o latură are 6 cm. Cât au celelalte laturi?\n"
          "Laturile opuse sunt congruente, deci două laturi au 6 cm.\n"
          "Celelalte două au împreună 30 − 2 · 6 = 18 cm, deci câte 9 cm."},
     }},
    // slot 3: 4.3
    {"Linia mijlocie",
     "Geometrie",
     {
         {"Linia mijlocie",
          "Linia mijlocie a unui triunghi unește mijloacele a două laturi. Orice triunghi are trei linii mijlocii.\n"
          "Teoremă: linia mijlocie este paralelă cu a treia latură și are lungimea egală cu jumătate din ea. Dacă M este mijlocul lui AB și N este mijlocul lui AC, atunci MN = BC : 2."},
         {"Triunghiul median",
          "Cele trei linii mijlocii împart triunghiul în patru triunghiuri congruente.\n"
          "Fiecare linie mijlocie are jumătate din latura paralelă cu ea, deci perimetrul triunghiului format de liniile mijlocii este jumătate din perimetrul triunghiului inițial."},
         {"Mediana",
          "Mediana unui triunghi unește un vârf cu mijlocul laturii opuse. Nu o confunda cu linia mijlocie, care unește mijloacele a două laturi.\n"
          "Cele trei mediane ale unui triunghi se intersectează într-un punct G, numit centrul de greutate."},
         {"Centrul de greutate",
          "Teoremă: centrul de greutate se află pe fiecare mediană la două treimi de vârf și la o treime de latura opusă. Pentru mediana AM: GA = (2/3) · AM și GM = (1/3) · AM, deci GA = 2 · GM.\n"
          "Atenție: raportul 2 : 1 se măsoară începând de la vârf."},
     },
     {
         {"Exemplu: linia mijlocie",
          "În triunghiul ABC, M și N sunt mijloacele laturilor AB și AC, iar BC = 18 cm. Cât este MN?\n"
          "MN este linie mijlocie, deci MN = BC : 2 = 18 : 2 = 9 cm.\n"
          "Invers: dacă MN = 9 cm, atunci BC = 2 · 9 = 18 cm."},
         {"Exemplu: triunghiul median",
          "Triunghiul ABC are laturile de 10 cm, 14 cm și 12 cm. Care este perimetrul triunghiului format de liniile mijlocii?\n"
          "Perimetrul lui ABC este 10 + 14 + 12 = 36 cm.\n"
          "Perimetrul triunghiului median este jumătate din el: 36 : 2 = 18 cm."},
         {"Exemplu: centrul de greutate",
          "Mediana AM a triunghiului ABC are 18 cm, iar G este centrul de greutate. Cât sunt GA și GM?\n"
          "GA = (2/3) · 18 = 12 cm.\n"
          "GM = (1/3) · 18 = 6 cm. Verificare: 12 + 6 = 18 cm."},
     }},
    // slot 4: 4.4
    {"Dreptunghiul",
     "Geometrie",
     {
         {"Definiție",
          "Dreptunghiul este un paralelogram cu un unghi drept. Orice dreptunghi este paralelogram, dar nu orice paralelogram este dreptunghi.\n"
          "Teoremă: într-un dreptunghi toate cele patru unghiuri sunt drepte (90°)."},
         {"Diagonalele",
          "Teoremă: diagonalele dreptunghiului sunt congruente: AC = BD. Ele se și înjumătățesc, deci OA = OB = OC = OD, iar triunghiurile OAB, OBC, OCD, ODA sunt isoscele.\n"
          "Reciproc: un paralelogram cu diagonalele congruente este dreptunghi."},
         {"Cum îl recunoaștem",
          "Este dreptunghi: un paralelogram cu un unghi drept; un paralelogram cu diagonalele congruente; un paralelogram cu două unghiuri consecutive congruente; un patrulater convex cu trei unghiuri drepte.\n"
          "Atenție: doar două unghiuri drepte nu ajung, figura poate fi un trapez dreptunghic."},
         {"Simetrie și unghiuri",
          "Dreptunghiul are două axe de simetrie: mediatoarele laturilor.\n"
          "Unghiul ascuțit și cel obtuz dintre diagonale sunt suplementare. În triunghiul isoscel AOB, unghiul din O este cel dintre diagonale, iar unghiurile de la bază sunt (180° minus unghiul din O) : 2."},
     },
     {
         {"Exemplu: diagonalele",
          "ABCD este dreptunghi, iar O este intersecția diagonalelor. Dacă AO = 7 cm, cât sunt AC și BD?\n"
          "Diagonalele se înjumătățesc: AC = 2 · AO = 14 cm.\n"
          "Diagonalele sunt congruente: BD = AC = 14 cm."},
         {"Exemplu: unghiurile",
          "Unghiul ascuțit dintre diagonalele unui dreptunghi este de 68°. Cât măsoară cel obtuz și unghiul dintre diagonală și latura mai scurtă?\n"
          "Cel obtuz: 180° − 68° = 112°.\n"
          "Triunghiul AOB este isoscel: (180° − 68°) : 2 = 56°."},
         {"Exemplu: un triunghi echilateral",
          "Diagonalele unui dreptunghi sunt de 18 cm și formează un unghi de 60°. Care este perimetrul triunghiului AOB?\n"
          "OA = OB = 18 : 2 = 9 cm, iar unghiul din O este 60°.\n"
          "Triunghiul AOB este echilateral: perimetrul este 3 · 9 = 27 cm."},
     }},
    // slot 5: 4.5
    {"Rombul și pătratul",
     "Geometrie",
     {
         {"Rombul",
          "Rombul este un paralelogram cu două laturi consecutive congruente.\n"
          "Teoremă: toate laturile unui romb sunt congruente. Reciproc: un patrulater cu toate laturile congruente este romb."},
         {"Diagonalele rombului",
          "Teoremă: diagonalele rombului sunt perpendiculare și sunt bisectoarele unghiurilor lui. Ele nu sunt congruente (decât la pătrat).\n"
          "Reciproc: un paralelogram cu diagonalele perpendiculare, sau cu o diagonală bisectoare, este romb."},
         {"Pătratul",
          "Pătratul are toate laturile congruente și toate unghiurile drepte. Este și romb, și dreptunghi, deci are toate proprietățile lor: diagonalele sunt congruente, perpendiculare și bisectoare.\n"
          "Triunghiurile OAB, OBC, OCD, ODA sunt dreptunghice isoscele. Pătratul are patru axe de simetrie."},
         {"Ierarhia",
          "Orice pătrat este romb și dreptunghi; rombul și dreptunghiul sunt paralelograme. Deci orice pătrat este romb, dar nu orice romb este pătrat.\n"
          "Un dreptunghi cu două laturi consecutive congruente, sau cu diagonalele perpendiculare, este pătrat. Un romb cu un unghi drept, sau cu diagonalele congruente, este pătrat."},
         {"Laturi și diagonale",
          "În romb, diagonalele se taie în O, perpendicular, în mijlocul lor. Triunghiul AOB este dreptunghic, cu catetele AO și BO (jumătățile diagonalelor), iar latura AB este ipotenuza: AB² = AO² + BO².\n"
          "În pătrat, diagonala este l√2 (teorema lui Pitagora), deci latura este d : √2."},
     },
     {
         {"Exemplu: unghiurile rombului",
          "ABCD este romb, iar diagonala AC formează cu latura AB un unghi de 35°. Cât măsoară unghiurile rombului?\n"
          "Diagonala este bisectoare: unghiul A = 2 · 35° = 70°.\n"
          "Unghiurile consecutive sunt suplementare: unghiul B = 180° − 70° = 110°."},
         {"Exemplu: latura rombului",
          "Diagonalele unui romb au 12 cm și 16 cm. Cât sunt latura și perimetrul?\n"
          "AO = 6 cm și BO = 8 cm. AB² = 6² + 8² = 36 + 64 = 100, deci AB = 10 cm.\n"
          "Perimetrul: 4 · 10 = 40 cm."},
         {"Exemplu: latura pătratului",
          "Diagonala unui pătrat are 10 cm. Cât este latura?\n"
          "Din teorema lui Pitagora: 10² = l² + l² = 2 · l², deci l² = 50.\n"
          "l = √50 = √(25 · 2) = 5√2 cm."},
     }},
    // slot 6: 4.6
    {"Trapezul",
     "Geometrie",
     {
         {"Definiție",
          "Trapezul este un patrulater convex cu două laturi opuse paralele, numite baze, și celelalte două laturi neparalele. Baza mare este AB, baza mică este CD, iar AD și BC sunt laturile neparalele.\n"
          "Bazele au lungimi diferite, altfel figura ar fi paralelogram."},
         {"Unghiurile",
          "Într-un trapez, unghiurile alăturate unei laturi neparalele sunt suplementare: A + D = 180° și B + C = 180°.\n"
          "Atenție: unghiurile A și B, de pe aceeași bază, nu sunt în general suplementare."},
         {"Tipuri de trapez",
          "Trapez dreptunghic: o latură neparalelă este perpendiculară pe baze (are două unghiuri drepte).\n"
          "Trapez isoscel: laturile neparalele sunt congruente.\n"
          "Înălțimea trapezului este un segment perpendicular pe ambele baze. Ea are aceeași lungime oriunde o ducem."},
         {"Linia mijlocie",
          "Linia mijlocie a trapezului unește mijloacele laturilor neparalele.\n"
          "Teoremă: este paralelă cu bazele și MN = (AB + CD) : 2.\n"
          "Segmentul ei cuprins între diagonale are lungimea PQ = (AB − CD) : 2."},
         {"Trapezul isoscel",
          "Într-un trapez isoscel unghiurile alăturate aceleiași baze sunt congruente (A = B și C = D), iar diagonalele sunt congruente. Are o axă de simetrie: mediatoarea bazelor.\n"
          "Înălțimile duse din vârfurile bazei mici lasă pe baza mare segmente de lungime (AB − CD) : 2. Cu teorema lui Pitagora aflăm înălțimea."},
     },
     {
         {"Exemplu: unghiurile",
          "În trapezul ABCD, cu AB și CD paralele, unghiul A = 70° și unghiul C = 100°. Cât măsoară B și D?\n"
          "A și D sunt alăturate laturii AD: D = 180° − 70° = 110°.\n"
          "B și C sunt alăturate laturii BC: B = 180° − 100° = 80°."},
         {"Exemplu: linia mijlocie",
          "Trapezul ABCD are bazele AB = 12 cm și CD = 6 cm. Cât sunt linia mijlocie MN și segmentul PQ dintre diagonale?\n"
          "MN = (12 + 6) : 2 = 9 cm.\n"
          "PQ = (12 − 6) : 2 = 3 cm."},
         {"Exemplu: înălțimea",
          "Trapezul isoscel ABCD are AB = 20 cm, CD = 8 cm și AD = BC = 10 cm. Cât este înălțimea?\n"
          "Proiecția laturii: (20 − 8) : 2 = 6 cm.\n"
          "h² = 10² − 6² = 100 − 36 = 64, deci h = 8 cm."},
     }},
    // slot 7: 4.7
    {"Perimetre și arii",
     "Geometrie",
     {
         {"Perimetre",
          "Perimetrul este suma lungimilor laturilor. Triunghi: P = a + b + c. Dreptunghi: P = 2 · (L + l). Pătrat și romb: P = 4 · l. Paralelogram: P = 2 · (a + b). Trapez: suma celor patru laturi.\n"
          "Înainte de calcul, exprimă toate lungimile în aceeași unitate: 1 m = 10 dm = 100 cm."},
         {"Aria",
          "Aria măsoară suprafața figurii. Dreptunghi: A = L · l. Pătrat: A = l².\n"
          "Paralelogram: A = baza · înălțimea corespunzătoare. Înălțimea este perpendiculară pe bază, nu este latura oblică.\n"
          "Figurile congruente au aceeași arie, dar două figuri cu aceeași arie nu sunt neapărat congruente."},
         {"Triunghiul",
          "Aria triunghiului: A = (latura · înălțimea corespunzătoare) : 2. O diagonală împarte paralelogramul în două triunghiuri congruente, deci fiecare are jumătate din arie.\n"
          "Triunghi dreptunghic: A = (cateta1 · cateta2) : 2. Înălțimea pe ipotenuză este h = (cateta1 · cateta2) : ipotenuza."},
         {"Romb și trapez",
          "Rombul: A = (d1 · d2) : 2, jumătate din produsul diagonalelor. Fiind paralelogram, are și A = latura · înălțimea.\n"
          "Trapezul: A = ((B + b) · h) : 2, adică linia mijlocie înmulțită cu înălțimea."},
         {"Greșeli frecvente",
          "Nu uita să împarți la doi la triunghi și la romb.\n"
          "Înălțimea nu este latura oblică.\n"
          "Perimetrul și aria sunt independente: două figuri pot avea aceeași arie și perimetre diferite.\n"
          "Aria se scrie în cm², perimetrul în cm."},
     },
     {
         {"Exemplu: paralelogramul",
          "Paralelogramul ABCD are baza AB = 10 cm, latura BC = 7 cm și înălțimea corespunzătoare lui AB de 6 cm. Care sunt aria și perimetrul?\n"
          "A = 10 · 6 = 60 cm².\n"
          "P = 2 · (10 + 7) = 34 cm."},
         {"Exemplu: rombul",
          "Un romb are diagonalele de 12 cm și 16 cm. Care sunt latura, perimetrul și aria?\n"
          "Latura: 6² + 8² = 100, deci 10 cm. Perimetrul: 4 · 10 = 40 cm.\n"
          "Aria: (12 · 16) : 2 = 96 cm²."},
         {"Exemplu: trapezul și unitățile",
          "Un trapez are bazele de 10 cm și 0,7 dm, iar înălțimea de 4 cm. Care este aria?\n"
          "0,7 dm = 7 cm.\n"
          "A = ((10 + 7) · 4) : 2 = 68 : 2 = 34 cm²."},
     }},
    // slot 8: 4.8
    {"Recapitulare U4",
     "Geometrie",
     {
         {"Ce garantează ce",
          "Paralelogram: laturile opuse paralele sau congruente, unghiurile opuse congruente, unghiurile consecutive suplementare, diagonalele care se înjumătățesc, sau două laturi opuse paralele și congruente.\n"
          "Dreptunghi: paralelogram cu un unghi drept sau cu diagonalele congruente.\n"
          "Romb: paralelogram cu două laturi consecutive congruente sau cu diagonalele perpendiculare.\n"
          "Pătrat: dreptunghi și romb în același timp."},
         {"Formule",
          "Unghiurile unui patrulater convex: 360°. Linia mijlocie în triunghi: BC : 2. Centrul de greutate: GA = (2/3) · AM.\n"
          "Linia mijlocie în trapez: (B + b) : 2.\n"
          "Arii: paralelogram baza · h; triunghi (baza · h) : 2; romb (d1 · d2) : 2; trapez ((B + b) · h) : 2."},
         {"Greșeli frecvente",
          "Unghiuri consecutive: suplementare; unghiuri opuse: congruente.\n"
          "Linia mijlocie nu este mediană.\n"
          "Raportul 2 : 1 al medianei se măsoară de la vârf.\n"
          "Doar două unghiuri drepte nu dau dreptunghi.\n"
          "Diagonalele rombului nu sunt congruente.\n"
          "La arii: nu uita împărțirea la doi."},
     },
     {
         {"Exemplu: ce figură este?",
          "Un paralelogram ABCD are diagonalele congruente și perpendiculare. Ce figură este?\n"
          "Diagonale congruente: dreptunghi. Diagonale perpendiculare: romb.\n"
          "Fiind și dreptunghi, și romb, ABCD este pătrat."},
         {"Exemplu: două unghiuri",
          "În paralelogramul ABCD, unghiul A este dublul unghiului B. Cât măsoară ele?\n"
          "A și B sunt consecutive: A + B = 180°, adică 2 · B + B = 180°.\n"
          "3 · B = 180°, deci B = 60° și A = 120°."},
         {"Exemplu: înălțimea pe ipotenuză",
          "Un triunghi dreptunghic are catetele de 6 cm și 8 cm și ipotenuza de 10 cm. Cât este înălțimea dusă pe ipotenuză?\n"
          "Aria: (6 · 8) : 2 = 24 cm².\n"
          "Aria este și (10 · h) : 2, deci h = 48 : 10 = 4,8 cm."},
     }},

// Lecțiile Unității 5 (cercul) în română, câte un LessonText pe slot (9..14: 5.1 - 5.6). Incluse după cele din Unitatea 4.
    // slot 9: 5.1
    {"Coarde și arce în cerc",
     "Geometrie",
     {
         {"Elementele cercului",
          "Cercul C(O, r) este mulțimea punctelor din plan aflate la distanța r de punctul O. O este centrul, iar r este raza; toate razele sunt congruente.\n"
          "Coarda este un segment cu ambele capete pe cerc (AB în figură). Coarda care trece prin centru este diametrul: este cea mai lungă coardă și are lungimea 2r.\n"
          "Unghiul la centru are vârful în O, de exemplu unghiul AOB."},
         {"Unghiul la centru și arcele",
          "Două puncte A și B împart cercul în două arce: arcul mic și arcul mare.\n"
          "Măsura arcului mic AB este egală cu măsura unghiului la centru AOB. Arcul mare are 360° minus arcul mic. Arcele care acoperă tot cercul însumează 360°.\n"
          "Capetele unui diametru determină două semicercuri de câte 180°.\n"
          "Atenție: dacă unghiul AOB = 70°, arcul mare are 290°, nu 70°."},
         {"Arce congruente, coarde congruente",
          "În același cerc (sau în cercuri cu razele egale), arcele congruente au coardele congruente, iar coardele congruente au arcele congruente.\n"
          "O dreaptă care trece prin centru și este perpendiculară pe o coardă o împarte în două părți egale: în figură M este mijlocul lui AB, deci AM = MB = AB : 2.\n"
          "Atenție: arce cu aceeași măsură sunt congruente doar dacă cercurile au raze egale."},
         {"Distanța de la centru la coardă",
          "Distanța de la centru la o coardă este lungimea perpendicularei OM duse din O pe coarda AB. Piciorul ei, M, este mijlocul lui AB.\n"
          "Coardele congruente ale unui cerc sunt egal depărtate de centru, iar coardele egal depărtate de centru sunt congruente.\n"
          "Cu cât coarda este mai lungă, cu atât este mai aproape de centru; pentru diametru distanța este 0."},
         {"Coarda cu teorema lui Pitagora",
          "Triunghiul OMA este dreptunghic în M, cu OA = r, OM = d și AM = AB : 2. Deci:\n"
          "d² + (AB : 2)² = r²\n"
          "Pentru coardă: AM = √(r² − d²), apoi AB = 2 · AM.\n"
          "Pentru distanță: d = √(r² − (AB : 2)²).\n"
          "Atenție: coarda este de două ori AM, nu AM. Iar d este mereu mai mic decât r."},
     },
     {
         {"Exemplu: cele două arce",
          "O este centrul unui cerc, iar A și B sunt puncte pe cerc, cu unghiul AOB = 70°. Află cele două arce AB.\n"
          "Arcul mic este egal cu unghiul la centru: 70°.\n"
          "Arcul mare are 360° − 70° = 290°.\n"
          "Verificare: 70° + 290° = 360°."},
         {"Exemplu: coarda",
          "Un cerc are raza r = 13 cm. O coardă AB se află la distanța OM = 5 cm de centrul O. Cât este AB?\n"
          "M este mijlocul lui AB, iar în triunghiul dreptunghic OMA:\n"
          "AM² = 13² − 5² = 169 − 25 = 144, deci AM = 12 cm.\n"
          "AB = 2 · 12 = 24 cm."},
         {"Exemplu: distanța",
          "Un cerc are raza de 10 cm și o coardă AB = 16 cm. La ce distanță de centru se află AB?\n"
          "AM = 16 : 2 = 8 cm.\n"
          "OM² = 10² − 8² = 100 − 64 = 36\n"
          "OM = 6 cm."},
     }},
    // slot 10: 5.2
    {"Unghiul înscris în cerc",
     "Geometrie",
     {
         {"Unghiul înscris",
          "Unghiul înscris în cerc are vârful pe cerc, iar laturile conțin coarde. În figură, unghiul BAC este înscris; el subîntinde arcul BC, adică arcul cuprins între laturile lui.\n"
          "Teoremă: măsura unui unghi înscris este jumătate din măsura arcului cuprins între laturi.\n"
          "unghiul BAC = arcul BC : 2, deci arcul BC = 2 · unghiul BAC.\n"
          "Este și jumătate din unghiul la centru BOC de pe același arc."},
         {"Semicercul și unghiurile congruente",
          "Dacă BC este diametru, arcul BC are 180°, deci unghiul BAC = 180° : 2 = 90°. Unghiul înscris într-un semicerc este drept.\n"
          "Unghiurile înscrise care cuprind același arc sunt congruente, pentru că fiecare este jumătate din acel arc. În figură, orice alt punct A de aceeași parte a lui BC dă același unghi BAC."},
         {"Patrulaterul înscris",
          "Un patrulater ABCD cu toate cele patru vârfuri pe un cerc este înscris în cerc (cercul este circumscris patrulaterului).\n"
          "Teoremă: unghiurile opuse ale unui patrulater înscris sunt suplementare.\n"
          "A + C = 180° și B + D = 180°\n"
          "De ce: unghiul A este jumătate din arcul BCD, unghiul C este jumătate din arcul BAD, iar cele două arce însumează 360°.\n"
          "Nu orice patrulater se poate înscrie într-un cerc."},
         {"Greșeli frecvente",
          "1. Unghi la centru sau înscris? Unghiul la centru este egal cu arcul; cel înscris este jumătate din arc.\n"
          "2. Arc mic sau arc mare? Unghiul înscris ia arcul dintre laturile lui. Dacă unghiul este obtuz, acel arc are peste 180°.\n"
          "3. Vârful de cealaltă parte a coardei BC dă unghiul suplementar: 180° − unghiul BAC.\n"
          "4. Împarte o singură dată: arc : 2 sau unghi · 2."},
     },
     {
         {"Exemplu: arcul",
          "Un unghi BAC de 65° este înscris într-un cerc de centru O. Află arcul BC și unghiul la centru BOC.\n"
          "Arcul este dublul unghiului înscris: arcul BC = 2 · 65° = 130°.\n"
          "Unghiul la centru este egal cu arcul său, deci unghiul BOC = 130°."},
         {"Exemplu: patrulaterul înscris",
          "ABCD este înscris într-un cerc, cu A = 78° și B = 105°. Află C și D.\n"
          "Unghiurile opuse sunt suplementare:\n"
          "C = 180° − 78° = 102°\n"
          "D = 180° − 105° = 75°\n"
          "Verificare: 78° + 105° + 102° + 75° = 360°."},
         {"Exemplu: diametrul",
          "Triunghiul ABC este înscris într-un cerc, iar BC este diametru. Unghiul B = 35°. Află A și C.\n"
          "Unghiul înscris într-un semicerc este drept: A = 90°.\n"
          "Suma unghiurilor unui triunghi este 180°:\n"
          "C = 180° − 90° − 35° = 55°."},
     }},
    // slot 11: 5.3
    {"Tangente dintr-un punct exterior",
     "Geometrie",
     {
         {"Dreapta și cercul",
          "Fie d distanța de la centrul O la o dreaptă și r raza cercului.\n"
          "Dacă d < r, dreapta este secantă: are două puncte comune cu cercul.\n"
          "Dacă d = r, dreapta este tangentă: are un singur punct comun cu cercul, punctul de tangență.\n"
          "Dacă d > r, dreapta este exterioară: nu are puncte comune.\n"
          "Ca să decizi, compară distanța cu raza."},
         {"Tangenta și raza",
          "Teoremă: tangenta este perpendiculară pe raza dusă în punctul de tangență.\n"
          "În figură, AC este tangentă în C, deci triunghiul OCA este dreptunghic în C. Cu teorema lui Pitagora:\n"
          "OA² = r² + AC²\n"
          "lungimea tangentei: AC = √(OA² − r²)\n"
          "Dintr-un punct exterior cercului se pot duce două tangente; dintr-un punct de pe cerc, una; dintr-un punct interior, nici una."},
         {"Două tangente dintr-un punct",
          "Teoremă: cele două tangente AC și AD duse dintr-un punct exterior A sunt congruente: AC = AD (triunghiurile dreptunghice OCA și ODA sunt congruente).\n"
          "Dreapta OA este bisectoarea unghiurilor CAD și COD și mediatoarea segmentului CD.\n"
          "Deci unghiul CAD + unghiul COD = 180°.\n"
          "Atenție: segmentele congruente sunt cele două tangente, nu o tangentă și o rază."},
         {"Patrulaterul circumscris",
          "Un patrulater ABCD cu toate cele patru laturi tangente la un cerc este circumscris cercului.\n"
          "Teoremă: AB + CD = BC + AD. Sumele laturilor opuse sunt egale.\n"
          "De ce: din fiecare vârf pornesc două segmente de tangentă congruente, notate x, y, z, t. Atunci AB + CD = x + y + z + t = BC + AD.\n"
          "Folosire: AD = AB + CD − BC.\n"
          "Atenție: adună laturile opuse, niciodată pe cele alăturate."},
         {"Aria triunghiului",
          "Cercul înscris într-un triunghi este tangent la toate trei laturile. Centrul lui se află la intersecția bisectoarelor, iar raza r este perpendiculară pe fiecare latură.\n"
          "A = p · r\n"
          "unde p este semiperimetrul, p = (a + b + c) : 2.\n"
          "De ce: cele trei triunghiuri cu vârful în centru au bazele a, b, c și înălțimea r.\n"
          "Atenție: p este jumătate din perimetru, nu perimetrul. Unitatea este cm²."},
     },
     {
         {"Exemplu: lungimea tangentei",
          "Un cerc are centrul O și raza de 8 cm. Dintr-un punct A cu OA = 17 cm se duc tangentele AC și AD. Află AC.\n"
          "Raza OC este perpendiculară pe tangenta AC.\n"
          "AC² = OA² − OC² = 17² − 8² = 289 − 64 = 225\n"
          "AC = 15 cm, și AD = 15 cm."},
         {"Exemplu: patrulaterul circumscris",
          "ABCD este circumscris unui cerc, cu AB = 9 cm, BC = 6 cm și CD = 7 cm. Află AD.\n"
          "AB + CD = BC + AD, deci 9 + 7 = 6 + AD.\n"
          "AD = 9 + 7 − 6 = 10 cm."},
         {"Exemplu: aria",
          "Un triunghi are laturile de 10 cm, 12 cm și 14 cm, iar cercul înscris are raza de 4 cm. Află aria.\n"
          "p = (10 + 12 + 14) : 2 = 18 cm.\n"
          "A = p · r = 18 · 4 = 72 cm²."},
     }},
    // slot 12: 5.4
    {"Poligoane regulate înscrise într-un cerc",
     "Geometrie",
     {
         {"Poligoane regulate",
          "Un poligon regulat are toate laturile congruente și toate unghiurile congruente. Exemple: triunghiul echilateral și pătratul.\n"
          "Un poligon regulat cu n laturi este înscris în cerc când vârfurile lui A1, A2, ..., An sunt pe cerc; ele împart cercul în n arce congruente. Centrul O al cercului este centrul poligonului.\n"
          "Atenție: rombul are laturile congruente, dar nu este regulat; dreptunghiul are unghiurile congruente, dar nu este regulat."},
         {"Unghiul la centru",
          "Fiecare latură a poligonului determină un arc de 360° : n, deci fiecare unghi la centru, de exemplu unghiul A1OA2, are\n"
          "360° : n\n"
          "Pentru n = 6 are 60°; pentru n = 10 are 36°.\n"
          "Triunghiurile OA1A2, OA2A3, ... sunt congruente și isoscele, cu două laturi egale cu raza.\n"
          "Verificare: cele n unghiuri la centru însumează 360°."},
         {"Unghiul poligonului",
          "Unghiul unui poligon regulat cu n laturi este\n"
          "(n − 2) · 180° : n\n"
          "iar suma tuturor unghiurilor lui este (n − 2) · 180°.\n"
          "Pentru pentagon: 3 · 180° : 5 = 108°.\n"
          "Dacă se dă unghiul, scrie ecuația și rezolv-o în n; n trebuie să fie număr natural.\n"
          "Atenție: nu-l confunda cu unghiul la centru 360° : n."},
         {"Hexagonul și apotema",
          "Într-un hexagon regulat înscris într-un cerc de rază r, unghiul la centru este 360° : 6 = 60°. Triunghiul OAB este isoscel și are un unghi de 60°, deci este echilateral: latura hexagonului este egală cu raza, AB = r. Fiecare unghi al hexagonului are 120°.\n"
          "Apotema este segmentul de la centrul O la mijlocul unei laturi: distanța de la O la acea latură.\n"
          "Aria unui poligon regulat = perimetrul · apotema : 2."},
     },
     {
         {"Exemplu: pentagonul",
          "Un pentagon regulat este înscris într-un cerc. Află unghiul la centru și unghiul lui.\n"
          "Unghiul la centru: 360° : 5 = 72°.\n"
          "Unghiul pentagonului: (5 − 2) · 180° : 5 = 540° : 5 = 108°."},
         {"Exemplu: câte laturi?",
          "Fiecare unghi al unui poligon regulat are 140°. Câte laturi are?\n"
          "(n − 2) · 180° : n = 140°\n"
          "180n − 360 = 140n, deci 40n = 360 și n = 9.\n"
          "Verificare: 7 · 180° : 9 = 140°. Este un poligon regulat cu 9 laturi."},
         {"Exemplu: hexagonul",
          "Un hexagon regulat este înscris într-un cerc de rază 6 cm. Află latura și perimetrul.\n"
          "Unghiul la centru este 360° : 6 = 60°, deci triunghiul OAB este echilateral.\n"
          "AB = OA = 6 cm.\n"
          "Perimetrul = 6 · 6 = 36 cm."},
     }},
    // slot 13: 5.5
    {"Lungimea cercului și aria discului",
     "Geometrie",
     {
         {"Lungimea cercului",
          "Raportul dintre lungimea unui cerc și diametrul lui este același pentru toate cercurile. Este numărul π (pi), aproximativ 3,14; nu este o fracție exactă.\n"
          "L = 2πr\n"
          "Exemplu: r = 5 cm dă L = 10π cm, adică aproximativ 31,4 cm.\n"
          "Atenție: folosește raza r (diametrul este 2r). Lasă π în răspuns dacă nu ți se cere aproximarea și rotunjește doar la sfârșit."},
         {"Aria discului",
          "A = πr²\n"
          "Exemplu: r = 5 cm dă A = 25π cm², adică aproximativ 78,5 cm².\n"
          "Atenție: lungimea se măsoară în cm, aria în cm². Ridică întâi r la pătrat, apoi înmulțește cu π: πr² nu este (πr)².\n"
          "Pentru r: din L, r = L : (2π); din A, r² = A : π."},
         {"Lungimea unui arc",
          "Un arc de n° este n/360 din tot cercul, deci lungimea lui este\n"
          "(n/360) · 2πr\n"
          "Semicercul (n = 180) are lungimea πr.\n"
          "Exemplu: n = 60°, r = 6 cm: (60/360) · 2π · 6 = 2π cm.\n"
          "Sectorul de cerc este partea din disc cuprinsă între două raze și arcul dintre ele (sectorul OAB din figură)."},
         {"Aria unui sector de cerc",
          "Sectorul de n° este n/360 din disc, deci aria lui este\n"
          "(n/360) · πr²\n"
          "Exemplu: n = 60°, r = 6 cm: (60/360) · π · 36 = 6π cm².\n"
          "Atenție: lungimea arcului are 2πr, aria sectorului are πr². Nu le confunda: lungimea este în cm, aria în cm²."},
         {"Coroana circulară",
          "Două cercuri cu același centru, de raze R (mare) și r (mică), delimitează o coroană circulară. Aria ei este discul mare minus discul mic:\n"
          "πR² − πr² = π(R² − r²)\n"
          "Exemplu: R = 5 cm, r = 3 cm: π(25 − 9) = 16π cm².\n"
          "Atenție: ridică întâi la pătrat, apoi scade; π(R − r)² este greșit.\n"
          "Dacă înlocuiești π cu 3,14, fă-o abia la sfârșit."},
     },
     {
         {"Exemplu: lungime și arie",
          "Un cerc are raza de 9 cm. Află lungimea cercului și aria discului.\n"
          "L = 2 · π · 9 = 18π cm\n"
          "A = π · 9² = 81π cm²\n"
          "Cu π luat 3,14, L este aproximativ 56,5 cm."},
         {"Exemplu: arc și sector",
          "Un cerc are raza de 6 cm. Află lungimea unui arc de 120° și aria sectorului de 120°.\n"
          "120° este 120 : 360 = 1/3 din cerc.\n"
          "Arcul: (1/3) · 2π · 6 = (1/3) · 12π = 4π cm.\n"
          "Sectorul: (1/3) · π · 36 = 12π cm²."},
         {"Exemplu: coroana circulară",
          "Două cercuri concentrice au razele R = 10 cm și r = 6 cm. Află aria coroanei circulare.\n"
          "A = π(R² − r²) = π(100 − 36)\n"
          "100 − 36 = 64\n"
          "A = 64π cm²."},
     }},
    // slot 14: 5.6
    {"Recapitulare U5",
     "Geometrie",
     {
         {"Unghiuri, arce, coarde",
          "Unghiul la centru = arcul său. Unghiul înscris = arcul său : 2. Unghiul înscris într-un semicerc are 90°. Unghiurile opuse ale unui patrulater înscris însumează 180°.\n"
          "Coarda: d² + (coarda : 2)² = r², unde d este distanța de la centru. Coardele congruente sunt egal depărtate de centru.\n"
          "Folosire: cu un arc sau un unghi, dublezi sau înjumătățești; cu o coardă și distanța ei, aplici Pitagora cu jumătatea coardei."},
         {"Tangente și poligoane",
          "Tangenta este perpendiculară pe rază, deci tangenta² = OA² − r². Cele două tangente din A sunt congruente. Patrulater circumscris: AB + CD = BC + AD. Triunghi: aria = p · r.\n"
          "Poligon regulat cu n laturi: unghiul la centru 360° : n; unghiul poligonului (n − 2) · 180° : n; la hexagon latura = r."},
         {"Lungimi și arii",
          "L = 2πr    A = πr²\n"
          "Arc: (n/360) · 2πr    Sector: (n/360) · πr²\n"
          "Coroană: π(R² − r²)\n"
          "Ce faci: 1. Se cere o lungime (cm) sau o arie (cm²)? 2. Rază sau diametru? 3. Lasă π; rotunjește doar la sfârșit. 4. Scrie unitățile.\n"
          "Greșeli tipice: unghi la centru cu unghi înscris, coarda întreagă în loc de jumătate, lungimea arcului cu aria sectorului."},
     },
     {
         {"Exemplu: arc și unghi",
          "Un unghi înscris BAC = 40° într-un cerc de rază 9 cm. Află lungimea arcului BC.\n"
          "Arcul BC = 2 · 40° = 80°.\n"
          "Lungimea = (80/360) · 2π · 9 = (2/9) · 18π = 4π cm."},
         {"Exemplu: coardă și tangentă",
          "Un cerc are raza de 10 cm. (a) O coardă se află la 6 cm de centru: cât este ea? (b) Se duce o tangentă din A, cu OA = 26 cm: cât este ea?\n"
          "(a) jumătatea = √(10² − 6²) = √64 = 8, coarda = 2 · 8 = 16 cm.\n"
          "(b) 26² − 10² = 676 − 100 = 576, tangenta = 24 cm."},
         {"Exemplu: poligonul",
          "Un poligon regulat înscris într-un cerc are unghiul la centru de 30°. Află n și unghiul poligonului.\n"
          "n = 360° : 30° = 12.\n"
          "Unghiul = (12 − 2) · 180° : 12 = 1800° : 12 = 150°."},
     }},
// Lecțiile Unității 6 în română, câte un LessonText pe slot (15..20): 6.1 până la 6.6. Incluse de lessons_ro.h după Unitatea 5.
    // slot 15: 6.1
    {"Segmente proporționale",
     "Geometrie",
     {
         {"Raportul a două segmente",
          "Raportul a două segmente este câtul lungimilor lor, exprimate în aceeași unitate de măsură: AB/CD. Este un număr fără unitate și nu depinde de unitatea aleasă.\n"
          "În figură, AM = 6 cm și MB = 4 cm, deci AM/MB = 6/4 = 3/2.\n"
          "Atenție: pentru AB = 3 cm și CD = 2 dm, transformă mai întâi: 2 dm = 20 cm, deci AB/CD = 3/20."},
         {"Segmente proporționale",
          "Patru segmente sunt proporționale dacă lungimile lor (în aceeași unitate) formează o proporție: AB/CD = MN/PQ. Verifici cu produsul mezilor și al extremilor: AB · PQ = CD · MN.\n"
          "Un șir de rapoarte egale, A1B1/C1D1 = A2B2/C2D2 = ... = k, are coeficientul de proporționalitate k.\n"
          "Exemplu: 2/3 = 6/9, pentru că 2 · 9 = 3 · 6 = 18."},
         {"Un punct pe segment",
          "Punctul M este pe segmentul AB și AM/MB = a/b. Atunci AB se împarte în a + b părți egale, iar AM are a dintre ele:\n"
          "AM = AB · a : (a + b) și MB = AB · b : (a + b).\n"
          "De asemenea AM/AB = a/(a + b), iar AM + MB = AB.\n"
          "Atenție: AM/MB compară parte cu parte, AM/AB compară parte cu întreg. Nu le amesteca."},
         {"Paralele echidistante",
          "Paralelele echidistante sunt drepte paralele aflate la distanțe egale una de alta. Dacă taie o secantă în segmente congruente, taie orice secantă în segmente congruente.\n"
          "Așa împarți un segment în părți proporționale cu numerele a, b, c: aduni a + b + c, apoi fiecare parte este lungimea · numărul ei : suma.\n"
          "Exemplu: 10 cm în raportul 2 : 3 dă 4 cm și 6 cm."},
     },
     {
         {"Exemplu: raportul",
          "Segmentul AB are 6 cm, iar segmentul CD are 0,4 dm. Cât este AB/CD?\n"
          "Unitățile diferă, deci transformăm: 0,4 dm = 4 cm.\n"
          "AB/CD = 6/4 = 3/2.\n"
          "Raportul nu are unitate: AB/CD = 3/2."},
         {"Exemplu: un punct pe segment",
          "M este pe segmentul AB = 20 cm și AM/MB = 3/2. Află AM și MB.\n"
          "AB se împarte în 3 + 2 = 5 părți.\n"
          "AM = 20 · 3 : 5 = 12 cm.\n"
          "MB = 20 − 12 = 8 cm.\n"
          "Verificare: 12/8 = 3/2."},
         {"Exemplu: părți proporționale",
          "Împarte un segment de 18 cm în părți proporționale cu 2, 3 și 4.\n"
          "Suma este 2 + 3 + 4 = 9, deci o parte este 18 : 9 = 2 cm.\n"
          "Părțile sunt 2 · 2 = 4 cm, 3 · 2 = 6 cm și 4 · 2 = 8 cm.\n"
          "Verificare: 4 + 6 + 8 = 18 cm. Părțile sunt 4 cm, 6 cm și 8 cm."},
     }},
    // slot 16: 6.2
    {"Teorema lui Thales",
     "Geometrie",
     {
         {"Teorema",
          "O paralelă la o latură a unui triunghi taie celelalte două laturi în segmente proporționale.\n"
          "În figură, D este pe AB, E este pe AC și DE este paralelă cu BC. Atunci AD/DB = AE/EC.\n"
          "(Manualul tratează și punctele D, E pe prelungirile laturilor; aici ele sunt pe laturi.)\n"
          "Cum o folosești: scrii proporția, apoi produsul mezilor și al extremilor îți dă segmentul necunoscut."},
         {"Proporțiile",
          "Din AD/DB = AE/EC rezultă proporțiile derivate: AD/AB = AE/AC și DB/AB = EC/AC.\n"
          "De asemenea AB = AD + DB și AC = AE + EC.\n"
          "Atenție: asociezi parte cu parte (AD cu DB, AE cu EC) sau parte cu întreg (AD cu AB, AE cu AC), la fel pe ambele laturi. AD/DB = AE/AC este greșit."},
         {"Figura X",
          "În figură, dreptele AB și CD se taie în O, iar AC este paralelă cu BD. Teorema lui Thales dă OA/OB = OC/OD.\n"
          "Fiecare raport compară cele două părți ale unei drepte prin O, în aceeași ordine: OA cu OB, OC cu OD.\n"
          "Atenție: OA/OB = OD/OC este greșit. Verifică ordinea literelor."},
         {"Teorema bisectoarei",
          "În figură, AD este bisectoarea unghiului A, iar D este pe BC. Bisectoarea împarte latura opusă în segmente proporționale cu celelalte două laturi: BD/DC = AB/AC.\n"
          "Deci BD = BC · AB : (AB + AC) și DC = BC · AC : (AB + AC).\n"
          "Atenție: BD se asociază cu AB, DC cu AC. Verificare: BD + DC = BC."},
     },
     {
         {"Exemplu: Thales",
          "DE este paralelă cu BC, cu D pe AB și E pe AC. AD = 4 cm, DB = 6 cm și AE = 6 cm. Află EC și AC.\n"
          "AD/DB = AE/EC, deci 4/6 = 6/EC.\n"
          "EC = 6 · 6 : 4 = 9 cm.\n"
          "AC = 6 + 9 = 15 cm."},
         {"Exemplu: figura X",
          "Dreptele AB și CD se taie în O, iar AC este paralelă cu BD. OA = 4 cm, OB = 6 cm și OC = 10 cm. Află OD.\n"
          "OA/OB = OC/OD, deci 4/6 = 10/OD.\n"
          "OD = 6 · 10 : 4 = 15 cm.\n"
          "Verificare: 4/6 = 2/3 și 10/15 = 2/3."},
         {"Exemplu: bisectoarea",
          "În triunghiul ABC, AB = 12 cm, AC = 18 cm și BC = 20 cm. Bisectoarea AD are D pe BC. Află BD și DC.\n"
          "BD/DC = AB/AC = 12/18 = 2/3.\n"
          "BD = 20 · 12 : 30 = 8 cm.\n"
          "DC = 20 − 8 = 12 cm. Verificare: 8/12 = 2/3."},
     }},
    // slot 17: 6.3
    {"Reciproca teoremei lui Thales",
     "Geometrie",
     {
         {"Enunțul",
          "În figură, D este pe AB și E este pe AC: poziții omoloage, amândouă pe laturi.\n"
          "Reciproca teoremei lui Thales: dacă D și E sunt în poziții omoloage și AD/DB = AE/EC, atunci DE este paralelă cu BC.\n"
          "Așa poți demonstra că două drepte sunt paralele folosind doar lungimi."},
         {"Cum verifici",
          "Calculezi rapoartele acelorași segmente pe ambele laturi: AD/AB și AE/AC, sau AD/DB și AE/EC. Mai întâi aduci toate lungimile la aceeași unitate.\n"
          "Rapoarte egale: DE este paralelă cu BC. Rapoarte diferite: DE nu este paralelă cu BC. Nu există „poate”.\n"
          "Simplifică fracțiile ca să le compari: 6/15 = 2/5."},
         {"Confuzii frecvente",
          "Parte cu întreg pe o latură și parte cu parte pe cealaltă nu se pot compara: AD/AB și AE/EC este o greșeală.\n"
          "Compară aceleași segmente, în aceeași ordine: AD cu AE, AB cu AC.\n"
          "Unități diferite: 1,5 dm și 6 cm nu sunt 1,5 și 6. Transformă mai întâi.\n"
          "Nu concluziona din zecimale rotunjite; compară fracții exacte."},
     },
     {
         {"Exemplu: paralele",
          "AB = 12 cm, AD = 4 cm, AC = 15 cm și AE = 5 cm, cu D pe AB și E pe AC. Este DE paralelă cu BC?\n"
          "AD/AB = 4/12 = 1/3.\n"
          "AE/AC = 5/15 = 1/3.\n"
          "Rapoartele sunt egale, deci DE este paralelă cu BC."},
         {"Exemplu: neparalele",
          "AD = 3 cm, DB = 6 cm, AE = 4 cm și EC = 7 cm. Este DE paralelă cu BC?\n"
          "AD/DB = 3/6 = 1/2.\n"
          "AE/EC = 4/7.\n"
          "1/2 nu este egal cu 4/7, deci DE nu este paralelă cu BC."},
         {"Exemplu: unități diferite",
          "AB = 1,5 dm, AD = 6 cm, AC = 20 cm și AE = 8 cm. Este DE paralelă cu BC?\n"
          "1,5 dm = 15 cm.\n"
          "AD/AB = 6/15 = 2/5 și AE/AC = 8/20 = 2/5.\n"
          "Rapoartele sunt egale, deci DE este paralelă cu BC."},
     }},
    // slot 18: 6.4
    {"Triunghiuri asemenea",
     "Geometrie",
     {
         {"Definiția",
          "Triunghiurile ABC și DEF sunt asemenea dacă au unghiurile congruente două câte două ȘI laturile proporționale: A = D, B = E, C = F și AB/DE = AC/DF = BC/EF = k.\n"
          "Ordinea literelor arată vârfurile corespondente: A cu D, B cu E, C cu F. Numărul k este raportul de asemănare.\n"
          "Atenție: „ABC asemenea cu DEF” și „ABC asemenea cu FDE” sunt afirmații diferite."},
         {"Teorema fundamentală",
          "T.F.A.: o paralelă la o latură a unui triunghi formează un triunghi asemenea cu cel dat. În figură, DE este paralelă cu BC, deci triunghiul ADE este asemenea cu triunghiul ABC.\n"
          "Prin urmare AD/AB = AE/AC = DE/BC.\n"
          "Ipoteza este aceeași ca la teorema lui Thales, dar concluzia privește toate laturile, inclusiv DE și BC. DE corespunde lui BC, nu lui DB."},
         {"Aflarea unei laturi",
          "Scrii rapoartele egale în ordinea literelor, apoi folosești produsul mezilor și al extremilor.\n"
          "Dacă triunghiul ABC este asemenea cu triunghiul DEF, cu AB = 6, DE = 4 și BC = 9, atunci 6/4 = 9/EF, deci EF = 9 · 4 : 6 = 6.\n"
          "Atenție: asociază laturile corespondente și folosește o singură unitate pentru toate lungimile."},
         {"Cazuri speciale",
          "Orice două triunghiuri echilaterale sunt asemenea: toate unghiurile lor au 60°, iar raportul laturilor este același pentru fiecare pereche.\n"
          "Dacă k = 1, laturile corespondente sunt egale, deci triunghiurile sunt congruente: congruența este un caz particular de asemănare.\n"
          "Triunghiurile asemenea pot avea mărimi diferite, dar nu forme diferite."},
     },
     {
         {"Exemplu: laturi",
          "Triunghiul ABC este asemenea cu triunghiul DEF. AB = 6 cm, BC = 9 cm, AC = 12 cm și DE = 4 cm. Află EF și DF.\n"
          "AB/DE = 6/4 = 3/2, iar BC/EF = AC/DF = 3/2.\n"
          "EF = 9 · 4 : 6 = 6 cm.\n"
          "DF = 12 · 4 : 6 = 8 cm."},
         {"Exemplu: T.F.A.",
          "DE este paralelă cu BC, cu D pe AB și E pe AC. AD = 3 cm, AB = 9 cm, BC = 15 cm și AC = 12 cm. Află DE și AE.\n"
          "Triunghiul ADE este asemenea cu triunghiul ABC, deci AD/AB = DE/BC = AE/AC = 3/9.\n"
          "DE = 3 · 15 : 9 = 5 cm.\n"
          "AE = 3 · 12 : 9 = 4 cm."},
         {"Exemplu: unghiuri",
          "Triunghiul ABC este asemenea cu triunghiul DEF. Unghiul A = 50° și unghiul B = 60°. Află unghiul F și latura care corespunde lui EF.\n"
          "C = 180° − (50° + 60°) = 70°.\n"
          "F corespunde lui C, deci F = 70°.\n"
          "EF este între E și F, care corespund lui B și C, deci EF corespunde lui BC."},
     }},
    // slot 19: 6.5
    {"Criteriile de asemănare",
     "Geometrie",
     {
         {"Criteriul UU",
          "Criteriul UU (unghi-unghi): dacă două unghiuri ale unui triunghi sunt congruente cu două unghiuri ale altui triunghi, triunghiurile sunt asemenea. Și al treilea unghi este egal (180° minus celelalte două).\n"
          "Dacă A = D și B = E, atunci triunghiul ABC este asemenea cu triunghiul DEF.\n"
          "Atenție: unghiurile egale trebuie să fie corespondente (A cu D, B cu E)."},
         {"Criteriul LUL",
          "Criteriul LUL (latură-unghi-latură): dacă două laturi ale unui triunghi sunt proporționale cu două laturi ale altui triunghi, iar unghiurile CUPRINSE între ele sunt congruente, triunghiurile sunt asemenea.\n"
          "Dacă AB/DE = AC/DF și A = D, atunci triunghiul ABC este asemenea cu triunghiul DEF.\n"
          "Atenție: unghiul trebuie să fie cuprins între cele două laturi proporționale. Un unghi necuprins între ele nu demonstrează nimic."},
         {"Criteriul LLL",
          "Criteriul LLL (latură-latură-latură): dacă cele trei laturi ale unui triunghi sunt proporționale cu cele trei laturi ale altui triunghi, triunghiurile sunt asemenea: AB/DE = AC/DF = BC/EF.\n"
          "Asociezi laturile după mărime: cea mai mică cu cea mai mică, cea mai mare cu cea mai mare.\n"
          "Folosește o singură unitate și compară fracții exacte."},
         {"Înălțimi, perimetre, arii",
          "Două triunghiuri asemenea, cu raportul de asemănare k, au:\n"
          "înălțimile corespunzătoare în raportul k,\n"
          "perimetrele în raportul k,\n"
          "ariile în raportul k².\n"
          "Deci k = 3 înseamnă perimetre de 3 ori mai mari, dar arii de 9 ori mai mari. Din raportul ariilor, k este rădăcina pătrată a lui. Atenție: raportul ariilor este k², nu k."},
         {"Aplicații",
          "Triunghiurile asemenea măsoară ce nu poți atinge. În același moment, un par și un copac stau vertical, iar razele soarelui sunt paralele, deci formează cu umbrele lor triunghiuri asemenea.\n"
          "Par de 2 m, umbră de 3 m. Umbra copacului: 12 m. Atunci înălțimea/2 = 12/3, deci înălțimea este 8 m.\n"
          "Mereu: găsește unghiurile egale, numește laturile corespondente, scrie proporția."},
     },
     {
         {"Exemplu: UU",
          "Triunghiul ABC are A = 45° și B = 75°. Triunghiul DEF are D = 45° și F = 60°. Sunt asemenea?\n"
          "C = 180° − (45° + 75°) = 60°.\n"
          "E = 180° − (45° + 60°) = 75°.\n"
          "A = D și B = E, deci, conform criteriului UU, triunghiul ABC este asemenea cu triunghiul DEF."},
         {"Exemplu: LUL",
          "În triunghiul ABC, AB = 4 cm, AC = 6 cm și unghiul A = 40°. DE = 8 cm, DF = 12 cm și unghiul D = 40°. BC = 5 cm. Află EF.\n"
          "AB/DE = 4/8 = 1/2 și AC/DF = 6/12 = 1/2, iar A = D este unghiul cuprins între ele.\n"
          "Conform criteriului LUL, triunghiurile sunt asemenea, cu k = 1/2.\n"
          "EF = 5 · 2 = 10 cm."},
         {"Exemplu: perimetru și arie",
          "Triunghiul DEF este asemenea cu triunghiul ABC, cu raportul 3 (fiecare latură a lui DEF este de 3 ori latura lui ABC). ABC are perimetrul 12 cm și aria 10 cm².\n"
          "Perimetrul: 12 · 3 = 36 cm.\n"
          "Aria: 10 · 3² = 10 · 9 = 90 cm².\n"
          "Perimetrul lui DEF este 36 cm, iar aria este 90 cm²."},
     }},
    // slot 20: 6.6
    {"Recapitularea unității 6",
     "Geometrie",
     {
         {"Ce teoremă folosesc?",
          "DE paralelă cu BC, D pe AB, E pe AC: părți ale laturilor, AD/DB = AE/EC (Thales). Dacă intervin DE sau BC: AD/AB = AE/AC = DE/BC (T.F.A.).\n"
          "Două drepte care se taie în O, AC paralelă cu BD: OA/OB = OC/OD.\n"
          "Bisectoarea AD: BD/DC = AB/AC.\n"
          "Pentru a demonstra paralelismul: reciproca, comparând rapoartele acelorași segmente.\n"
          "Punctul M pe AB cu AM/MB = a/b: AM = AB · a : (a + b)."},
         {"Triunghiuri asemenea",
          "Pentru a demonstra asemănarea: UU (două unghiuri congruente), LUL (două laturi proporționale și unghiul cuprins între ele) sau LLL (trei laturi proporționale).\n"
          "Atunci laturile corespondente sunt proporționale, în ordinea literelor, cu raportul k.\n"
          "Înălțimi corespunzătoare: raportul k. Perimetre: raportul k. Arii: raportul k²."},
         {"Greșeli frecvente",
          "Aceeași unitate înainte de orice raport.\n"
          "Parte și întreg: AD/DB nu este AD/AB.\n"
          "Vârfuri corespondente după ordinea literelor.\n"
          "La LUL, unghiul este cuprins între cele două laturi.\n"
          "Raportul ariilor este k², iar k este rădăcina pătrată a lui.\n"
          "Rapoarte diferite înseamnă că dreptele nu sunt paralele."},
     },
     {
         {"Exemplu: Thales și T.F.A.",
          "DE este paralelă cu BC, cu D pe AB și E pe AC. AD = 2 cm, DB = 3 cm, AE = 4 cm și BC = 15 cm. Află EC și DE.\n"
          "AD/DB = AE/EC, deci EC = 3 · 4 : 2 = 6 cm.\n"
          "AB = 2 + 3 = 5 cm, deci DE/BC = AD/AB = 2/5.\n"
          "DE = 15 · 2 : 5 = 6 cm."},
         {"Exemplu: reciproca",
          "AD = 2 cm, AB = 6 cm, AE = 3 cm și AC = 10 cm, cu D pe AB și E pe AC. Este DE paralelă cu BC?\n"
          "AD/AB = 2/6 = 1/3.\n"
          "AE/AC = 3/10.\n"
          "1/3 nu este egal cu 3/10, deci DE nu este paralelă cu BC."},
         {"Exemplu: LLL și arii",
          "Triunghiul ABC are laturile de 3 cm, 4 cm și 5 cm, iar triunghiul DEF are laturile de 9 cm, 12 cm și 15 cm. Sunt asemenea? Compară ariile.\n"
          "3/9 = 4/12 = 5/15 = 1/3, deci, conform criteriului LLL, sunt asemenea.\n"
          "Aria lui ABC: (3 · 4) : 2 = 6 cm². Cu k = 3: 6 · 9 = 54 cm².\n"
          "Verificare: (9 · 12) : 2 = 54 cm²."},
     }},
