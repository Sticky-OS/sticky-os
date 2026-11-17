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
