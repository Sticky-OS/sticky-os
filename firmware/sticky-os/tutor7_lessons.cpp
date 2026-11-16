// Tutor7's lessons: see tutor7_lessons.h.
#include "tutor7_lessons.h"

namespace t7 {

// Lesson 1: Teorema lui Pitagora. Concepts, all from the 7th-grade textbooks' unit on
// metric relations in the right triangle: the right triangle and its parts, the theorem,
// the equivalent forms, the converse, the right isosceles triangle, the square's diagonal,
// the equilateral triangle's height. The worked examples use numbers of our own.
static const Lesson PITAGORA = {
    "Teorema lui Pitagora",
    "Geometrie",
    5,
    {
        {"Triunghiul dreptunghic",
         "Un triunghi dreptunghic are un unghi drept, de 90°.\n"
         "Latura opusă unghiului drept se numește ipotenuză și este cea mai lungă latură a triunghiului. "
         "Celelalte două laturi se numesc catete.\n"
         "În triunghiul ABC din figură, unghiul A este drept: BC este ipotenuza, iar AB și AC sunt catetele.",
         FIG_RIGHT_NAMED},
        {"Teorema lui Pitagora",
         "Într-un triunghi dreptunghic, pătratul ipotenuzei este egal cu suma pătratelor catetelor.\n"
         "Pentru triunghiul ABC, cu A = 90°:\n"
         "BC² = AB² + AC²\n"
         "De aici putem afla orice latură: BC = √(AB² + AC²), AB = √(BC² − AC²), AC = √(BC² − AB²).",
         FIG_RIGHT_NAMED},
        {"Cum folosim teorema",
         "1. Găsește unghiul drept și spune care este ipotenuza.\n"
         "2. Scrie relația lui Pitagora pentru triunghiul respectiv.\n"
         "3. Înlocuiește laturile cunoscute și calculează.\n"
         "4. Extrage rădăcina pătrată și scoate factorii de sub radical, dacă se poate: √32 = √(16·2) = 4√2.\n"
         "Ipotenuza este mereu mai lungă decât oricare dintre catete.",
         -1},
        {"Reciproca teoremei",
         "Dacă într-un triunghi suma pătratelor a două laturi este egală cu pătratul celei de-a treia laturi, "
         "atunci triunghiul este dreptunghic.\n"
         "Unghiul drept este opus celei mai lungi laturi. De aceea, la verificare, luăm cea mai lungă latură drept ipotenuză.\n"
         "Atenție: toate laturile trebuie să fie exprimate în aceeași unitate de măsură.",
         -1},
        {"Cazuri speciale",
         "Triunghiul dreptunghic isoscel, cu catetele egale cu c, are ipotenuza c√2. "
         "De aceea diagonala unui pătrat cu latura l este l√2.\n"
         "Triunghiul echilateral cu latura l are înălțimea l√3/2: înălțimea îl împarte în două triunghiuri dreptunghice, "
         "cu ipotenuza l și o catetă l/2.",
         -1},
    },
    3,
    {
        {"Exemplu: aflăm ipotenuza",
         "În triunghiul dreptunghic ABC, cu A = 90°, avem AB = 9 cm și AC = 40 cm. Cât este BC?\n"
         "Ipotenuza este BC, deci:\n"
         "BC² = AB² + AC² = 9² + 40² = 81 + 1600 = 1681\n"
         "BC = √1681 = 41 cm.",
         -1},
        {"Exemplu: aflăm o catetă",
         "În triunghiul dreptunghic ABC, cu A = 90°, avem BC = 9 cm și AB = 7 cm. Cât este AC?\n"
         "Ipotenuza este BC, deci:\n"
         "AC² = BC² − AB² = 9² − 7² = 81 − 49 = 32\n"
         "AC = √32 = √(16·2) = 4√2 cm.",
         -1},
        {"Exemplu: este triunghiul dreptunghic?",
         "Un triunghi are laturile de 20 cm, 21 cm și 29 cm. Este dreptunghic?\n"
         "Cea mai lungă latură este 29 cm, deci o încercăm drept ipotenuză:\n"
         "20² + 21² = 400 + 441 = 841, iar 29² = 841.\n"
         "Valorile sunt egale, deci triunghiul este dreptunghic, cu unghiul drept opus laturii de 29 cm.",
         -1},
    },
    7,
    {
        {K_HYPOTENUSE, 1},
        {K_LEG, 1},
        {K_HYPOTENUSE, 2},
        {K_LEG, 2},
        {K_CONVERSE, 3},
        {K_SQUARE_DIAGONAL, 3},
        {K_EQUILATERAL_HEIGHT, 3},
    },
};

const Lesson &lesson(int i) { return PITAGORA; }

}  // namespace t7
