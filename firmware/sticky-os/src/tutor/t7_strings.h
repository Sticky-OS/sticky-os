// Tutor's exercise words, in English: T7S(TX_ID, "English"), one row a string. English is the master:
// it makes the ids (tutor7_text.h) and is what a language shows when it hasn't got a string (but a
// language shows Tutor's content only when it has all of it: see t7::contentLang). Each language
// has a file beside this one (ro.h) of {TX_ID, "translation"} pairs, in any order.
//
// The exercises are made from numbers (tutor7_core.cpp, tutor7_u4.cpp): most rows are printf formats,
// and the comment after each says where it shows and what its placeholders get, in order. A
// translation keeps the same placeholders in the same order (firmware/test/i18n/check.py checks).
//
// Notation (English): × for times, ÷ for divided by, never "/" for a division (a fraction like 2/3
// is fine: verify() reads "/" as part of a number); a decimal point; ° ² − √. The fonts have no angle,
// parallel or perpendicular signs: "angle A", "AB and CD are parallel". √ is followed by its radicand
// (digits) or a parenthesised expression; the screen draws the bar. Hints and the reasons of wrong
// answers add no numbers of their own: the generator refuses an exercise whose hint shows the answer.
// Buffers (tutor7_core.h): a question 320 bytes, an answer 80, a hint or a reason 280, a solution 560,
// formatted; firmware/test/tutor7 makes every exercise in every language and fails if one doesn't fit.

// ---- Shared
T7S(TX_LIST_AND, " and ")  // between list items
T7S(TX_LIST_COMMA, ", ")  // between list items

// ---- On the exercise screens
T7S(TX_YOU_CHOSE, "You chose: %s")  // the wrong-answer screen, first line; %s the option
T7S(TX_RIGHT_ANSWER, "The right answer: %s")  // after the solution when the student gave up; %s the option
T7S(TX_FIG_AREA, "Area = ?")  // a label on the figure
T7S(TX_FIG_SIDE, "s = ?")  // a label on the figure

// ---- Pythagoras: find the hypotenuse (K_HYPOTENUSE)
T7S(TX_HYP_Q, "In right triangle ABC, angle A = 90°, AB = %d cm and AC = %d cm. How long is the hypotenuse BC?")  // the question; %d ab, %d ac
T7S(TX_HYP_SLIP1, "You added the legs' lengths and took the square root. The Pythagorean theorem adds the squares of the legs, not their lengths: BC² = AB² + AC².")  // a wrong answer: why
T7S(TX_HYP_SLIP2, "You squared only one leg. Square both legs: BC² = AB² + AC².")  // a wrong answer: why
T7S(TX_HYP_SLIP3, "You subtracted the squares. For the hypotenuse, add the squares of the legs: BC² = AB² + AC².")  // a wrong answer: why
T7S(TX_HYP_SLIP4, "You forgot to take the square root. The formula gives BC², not BC: BC = √(AB² + AC²).")  // a wrong answer: why
T7S(TX_HYP_SLIP5, "You added the legs' lengths. The Pythagorean theorem uses their squares: BC² = AB² + AC².")  // a wrong answer: why
T7S(TX_HYP_SLIP6, "You picked the longer leg. The hypotenuse is always longer than either leg.")  // a wrong answer: why
T7S(TX_HYP_HINT1, "The hypotenuse is the side opposite the right angle. Which side is it in triangle ABC?")  // hint 1
T7S(TX_HYP_HINT2, "Write the Pythagorean theorem for this triangle: BC² = AB² + AC². Put in the given values for AB and AC and work it out.")  // hint 2
T7S(TX_HYP_SOL, "BC² = AB² + AC² = %d² + %d² = %d + %d = %lld, so BC = %s cm.")  // the worked solution; %d ab, %d ac, %d ab * ab, %d ac * ac, %lld (long long)n, %s rt

// ---- Pythagoras: find a leg (K_LEG)
T7S(TX_LEG_Q, "In right triangle ABC, angle A = 90°, the hypotenuse BC = %d cm and leg AB = %d cm. How long is leg AC?")  // the question; %d bc, %d ab
T7S(TX_LEG_SLIP1, "You subtracted the lengths and took the square root. The Pythagorean theorem works with squares: AC² = BC² - AB².")  // a wrong answer: why
T7S(TX_LEG_SLIP2, "You squared only the hypotenuse. Square both sides: AC² = BC² - AB².")  // a wrong answer: why
T7S(TX_LEG_SLIP3, "You added the squares, as for the hypotenuse. The hypotenuse is the longest side, and BC² = AB² + AC², so AC² = BC² - AB².")  // a wrong answer: why
T7S(TX_LEG_SLIP4, "You forgot to take the square root. The difference of the squares is AC², not AC.")  // a wrong answer: why
T7S(TX_LEG_SLIP5, "You subtracted the lengths. The Pythagorean theorem uses the squares: AC² = BC² - AB².")  // a wrong answer: why
T7S(TX_LEG_SLIP6, "That is the length of the hypotenuse. Leg AC is shorter than the hypotenuse.")  // a wrong answer: why
T7S(TX_LEG_HINT1, "The hypotenuse is BC, the side opposite the right angle. From BC² = AB² + AC², find AC².")  // hint 1
T7S(TX_LEG_HINT2, "AC² = BC² - AB². Put in the values, find the difference, then take the square root.")  // hint 2
T7S(TX_LEG_SOL, "AC² = BC² - AB² = %d² - %d² = %d - %d = %lld, so AC = %s cm.")  // the worked solution; %d bc, %d ab, %d bc * bc, %d ab * ab, %lld (long long)n, %s rt

// ---- Pythagoras: is it a right triangle? (K_CONVERSE); x < y < z the sides sorted
T7S(TX_CONV_Q, "A triangle has sides of %d cm, %d cm and %d cm. Is it a right triangle?")  // the question; %d sides[0], %d sides[1], %d sides[2]
T7S(TX_CONV_OPT_YES, "Yes, the right angle is opposite the %d cm side")  // an answer; %d the side the right angle is opposite (the longest, or the shortest for a wrong answer)
T7S(TX_CONV_OPT_NO, "No, it is not a right triangle")  // an answer
T7S(TX_CONV_WHY1, "The right angle is always opposite the longest side (the hypotenuse), not the shortest.")  // a wrong answer: why
T7S(TX_CONV_WHY2, "Check: %d² + %d² = %d + %d = %d and %d² = %d. Compare the two results: what does the converse of the Pythagorean theorem say in this case?")  // a wrong answer: why; %d x, %d y, %d x * x, %d y * y, %d x * x + y * y, %d z, %d z * z
T7S(TX_CONV_WHY3, "Check: %d² + %d² = %d, but %d² = %d. The values are not equal, so the angle is not a right angle.")  // a wrong answer: why; %d x, %d y, %d x * x + y * y, %d z, %d z * z
T7S(TX_CONV_WHY4, "The right angle would be opposite the longest side, not the shortest. Also, %d² + %d² = %d, but %d² = %d: the values are not equal.")  // a wrong answer: why; %d x, %d y, %d x * x + y * y, %d z, %d z * z
T7S(TX_CONV_HINT1, "For the converse of the Pythagorean theorem, try the longest side as the hypotenuse. Which side is it?")  // hint 1
T7S(TX_CONV_HINT2, "Add the squares of the other two sides and compare the sum with the square of the longest side.")  // hint 2
T7S(TX_CONV_SOL1, "The longest side is %d cm. %d² + %d² = %d + %d = %d and %d² = %d. The values are equal, so it is a right triangle (the right angle is opposite the %d cm side).")  // the worked solution; %d z, %d x, %d y, %d x * x, %d y * y, %d x * x + y * y, %d z, %d z * z, %d z
T7S(TX_CONV_SOL2, "The longest side is %d cm. %d² + %d² = %d + %d = %d, but %d² = %d. The values are different, so it is not a right triangle.")  // the worked solution; %d z, %d x, %d y, %d x * x, %d y * y, %d x * x + y * y, %d z, %d z * z

// ---- Pythagoras: a square's diagonal (K_SQUARE_DIAGONAL); l the side
T7S(TX_SQD_Q, "A square has a side of %d cm. How long is its diagonal?")  // the question; %d l
T7S(TX_SQD_SLIP1, "You doubled the side. The diagonal is the hypotenuse of an isosceles right triangle with legs of %d cm: d² = %d² + %d², so doubling the side doesn't give it.")  // a wrong answer: why; %d l, %d l, %d l
T7S(TX_SQD_SLIP2, "√3 belongs to the height of an equilateral triangle, not here. The diagonal is the hypotenuse of an isosceles right triangle with legs of %d cm: d² = %d² + %d².")  // a wrong answer: why; %d l, %d l, %d l
T7S(TX_SQD_SLIP3, "That is half the diagonal. The whole diagonal is the hypotenuse: d² = %d² + %d², not half of it.")  // a wrong answer: why; %d l, %d l
T7S(TX_SQD_SLIP4, "That is the area of the square, not the length of the diagonal. Find the diagonal with the Pythagorean theorem: d² = %d² + %d².")  // a wrong answer: why; %d l, %d l
T7S(TX_SQD_HINT1, "The diagonal splits the square into two right triangles. In one of them, which sides are the legs and which is the hypotenuse?")  // hint 1
T7S(TX_SQD_HINT2, "The legs are sides of the square, %d cm each, and the diagonal d is the hypotenuse. Use the Pythagorean theorem: d² = %d² + %d².")  // hint 2; %d l, %d l, %d l
T7S(TX_SQD_SOL, "The diagonal is the hypotenuse of an isosceles right triangle with legs of %d cm: d² = %d² + %d² = %d + %d = %d, so d = %s cm.")  // the worked solution; %d l, %d l, %d l, %d l * l, %d l * l, %d 2 * l * l, %s rt

// ---- Pythagoras: an equilateral triangle's height (K_EQUILATERAL_HEIGHT); l the side, h2 half of it
T7S(TX_EQH_Q, "Equilateral triangle ABC has sides of %d cm. How long is the height AD?")  // the question; %d l
T7S(TX_EQH_SLIP1, "You forgot to divide the side by 2. In right triangle ABD, leg BD is half of BC: BD = %d cm, then AD² = AB² - BD².")  // a wrong answer: why; %d h2
T7S(TX_EQH_SLIP2, "That is the length of BD, half the side, not the height AD. The height AD is the other leg of triangle ABD.")  // a wrong answer: why
T7S(TX_EQH_SLIP3, "You divided by 2 twice. Halve the side only once (BD = %d cm); then AD² = AB² - BD² = %d² - %d².")  // a wrong answer: why; %d h2, %d l, %d h2
T7S(TX_EQH_SLIP4, "This is an equilateral triangle, not a square. Work out AD² = AB² - BD² = %d² - %d² carefully, then take the square root.")  // a wrong answer: why; %d l, %d h2
T7S(TX_EQH_HINT1, "The height AD is perpendicular to BC, and D is the midpoint of BC. What kind of triangle is ABD?")  // hint 1
T7S(TX_EQH_HINT2, "In right triangle ABD, the hypotenuse is AB = %d cm and one leg is BD = %d cm. Use the Pythagorean theorem to find AD.")  // hint 2; %d l, %d h2
T7S(TX_EQH_SOL, "D is the midpoint of BC, so BD = %d cm. In right triangle ABD: AD² = AB² - BD² = %d² - %d² = %d - %d = %d, so AD = %s cm.")  // the worked solution; %d h2, %d l, %d h2, %d l * l, %d h2 * h2, %d 3 * h2 * h2, %s rt

// ---- 4.1: a convex quadrilateral's fourth angle (K_Q_FOURTH_ANGLE)
T7S(TX_Q4_Q, "Convex quadrilateral ABCD has angle %s = %d°, angle %s = %d° and angle %s = %d°. What is the measure of angle %s?")  // the question; %s gl[0], %d g[0], %s gl[1], %d g[1], %s gl[2], %d g[2], %s V4[u]
T7S(TX_Q4_SLIP1, "You added the three angles, but their sum is not the answer. All four angles add up to 360°, so the missing angle is what brings the sum up to 360°.")  // a wrong answer: why
T7S(TX_Q4_SLIP2, "You used 180°, the angle sum of a triangle. In a convex quadrilateral the angles add up to 360°.")  // a wrong answer: why
T7S(TX_Q4_SLIP3, "You left out one of the given angles. Subtract all three known angles from 360°.")  // a wrong answer: why
T7S(TX_Q4_HINT1, "The four angles of a convex quadrilateral always have the same sum, whatever the quadrilateral. Remember what it is.")  // hint 1
T7S(TX_Q4_HINT2, "Add the three given angles, then subtract their sum from the total of all four angles.")  // hint 2
T7S(TX_Q4_SOL, "The angles of a convex quadrilateral add up to 360°. Angle %s = 360° − (%d° + %d° + %d°) = 360° − %d° = %d°.")  // the worked solution; %s V4[u], %d g[0], %d g[1], %d g[2], %d S, %d a[u]

// ---- 4.1: angles in a given ratio, the largest (K_Q_PROPORTIONAL)
T7S(TX_QPROP_Q, "The angles of a convex quadrilateral are proportional to the numbers %d, %d, %d and %d. What is the measure of the largest angle?")  // the question; %d r[0], %d r[1], %d r[2], %d r[3]
T7S(TX_QPROP_SLIP1, "You split the sum into four equal parts, but the angles are not equal: they are proportional to the given numbers.")  // a wrong answer: why
T7S(TX_QPROP_SLIP2, "You used 180° instead of the angle sum of a convex quadrilateral.")  // a wrong answer: why
T7S(TX_QPROP_SLIP3, "The given numbers are only the proportions, not the angles in degrees. First find how much one part is worth.")  // a wrong answer: why
T7S(TX_QPROP_SLIP4, "That is the size of just one part. The largest angle is made of several such parts.")  // a wrong answer: why
T7S(TX_QPROP_SLIP5, "That is the sum of the other three angles, not the measure of the largest one.")  // a wrong answer: why
T7S(TX_QPROP_HINT1, "The four angles have the same sum in every convex quadrilateral. Split that sum into as many equal parts as the given numbers add up to.")  // hint 1
T7S(TX_QPROP_HINT2, "Find how much one part is worth (the angle sum divided by the sum of the given numbers), then multiply by the largest number.")  // hint 2
T7S(TX_QPROP_SOL, "The angles add up to 360°. Sum of the numbers: %d + %d + %d + %d = %d, so one part is 360° ÷ %d = %d°. Largest angle: %d × %d° = %d°.")  // the worked solution; %d r[0], %d r[1], %d r[2], %d r[3], %d S, %d S, %d part, %d m, %d part, %d big

// ---- 4.2: a parallelogram's angles (K_PAR_ANGLE)
T7S(TX_PANG_Q, "In parallelogram ABCD, angle A measures %d°. What is the measure of angle %s?")  // the question; %d A, %s name[which]
T7S(TX_PANG_SLIP1, "Angle C is opposite angle A, not consecutive with it. Opposite angles of a parallelogram are congruent.")  // a wrong answer: why
T7S(TX_PANG_SLIP2, "You subtracted from 90°. The angles of a parallelogram have no such relation.")  // a wrong answer: why
T7S(TX_PANG_SLIP3, "You subtracted from 360°. 360° is the sum of all four angles, and angle C is opposite angle A.")  // a wrong answer: why
T7S(TX_PANG_SLIP4, "That is the sum of angle B and angle D, not the measure of one angle.")  // a wrong answer: why
T7S(TX_PANG_SLIP5, "Angle %s is consecutive with angle A (they share a side). Consecutive angles of a parallelogram are supplementary, not equal.")  // a wrong answer: why; %s name[which]
T7S(TX_PANG_SLIP6, "You subtracted from 90°. Consecutive angles of a parallelogram add up to a different number of degrees.")  // a wrong answer: why
T7S(TX_PANG_SLIP7, "You subtracted from 360°. 360° is the sum of all four angles; two consecutive angles have a smaller sum.")  // a wrong answer: why
T7S(TX_PANG_HINT1, "Look at where the angle you need is compared with angle A: is it consecutive (sharing a side with it) or opposite?")  // hint 1
T7S(TX_PANG_HINT2, "Opposite angles of a parallelogram are congruent, and two consecutive angles are supplementary (they add up to 180°).")  // hint 2
T7S(TX_PANG_SOL1, "Angle C is opposite angle A, and opposite angles of a parallelogram are congruent: angle C = angle A = %d°.")  // the worked solution; %d A
T7S(TX_PANG_SOL2, "Angle %s is consecutive with angle A, so the two are supplementary: angle %s = 180° − %d° = %d°.")  // the worked solution; %s name[which], %s name[which], %d A, %d ans

// ---- 4.2: a parallelogram's perimeter, or a side from it (K_PAR_PERIMETER)
T7S(TX_PPER_Q1, "Parallelogram ABCD has AB = %d cm and BC = %d cm. What is its perimeter?")  // the question; %d a, %d b
T7S(TX_PPER_SLIP1, "That is the sum of two adjacent sides. The perimeter adds all four sides, and opposite sides are congruent.")  // a wrong answer: why
T7S(TX_PPER_SLIP2, "You multiplied the side lengths. The perimeter is the sum of the side lengths, not their product.")  // a wrong answer: why
T7S(TX_PPER_SLIP3, "You counted one side only once. Each length appears twice, because opposite sides are congruent.")  // a wrong answer: why
T7S(TX_PPER_SLIP4, "You took all the sides to be as long as the longest one. In a general parallelogram only opposite sides are congruent.")  // a wrong answer: why
T7S(TX_PPER_HINT1_1, "In a parallelogram, opposite sides are congruent. How many sides does the perimeter add up, and how long are they?")  // hint 1
T7S(TX_PPER_HINT2_1, "The perimeter is twice the sum of two adjacent sides.")  // hint 2
T7S(TX_PPER_SOL1, "Opposite sides are congruent, so the perimeter is 2 × (AB + BC) = 2 × (%d + %d) = 2 × %d = %d cm.")  // the worked solution; %d a, %d b, %d a + b, %d P
T7S(TX_PPER_Q2, "The perimeter of parallelogram ABCD is %d cm, and side AB is %d cm. How long is side BC?")  // the question; %d P, %d a
T7S(TX_PPER_SLIP5, "That is the sum of AB and BC (half the perimeter), not the length of BC.")  // a wrong answer: why
T7S(TX_PPER_SLIP6, "You subtracted only one side from the perimeter, but AB appears twice in the perimeter.")  // a wrong answer: why
T7S(TX_PPER_SLIP7, "You subtracted the two sides equal to AB, but two sides equal to BC are left: divide the result by two.")  // a wrong answer: why
T7S(TX_PPER_SLIP8, "You divided the perimeter by four, as for a rhombus. In a general parallelogram, adjacent sides are not equal.")  // a wrong answer: why
T7S(TX_PPER_HINT1_2, "The perimeter is twice the sum of two adjacent sides. What is that sum?")  // hint 1
T7S(TX_PPER_HINT2_2, "From the sum of two adjacent sides, subtract the side you know.")  // hint 2
T7S(TX_PPER_SOL2, "The perimeter is 2 × (AB + BC), so AB + BC = %d ÷ 2 = %d cm. Then BC = %d − %d = %d cm.")  // the worked solution; %d P, %d P / 2, %d P / 2, %d a, %d b

// ---- 4.2: a parallelogram's diagonals bisect each other (K_PAR_DIAGONAL)
T7S(TX_PDIAG_Q, "The diagonals of parallelogram ABCD meet at point O. We know AO = %d cm and BO = %d cm. How long is %s?")  // the question; %d ao, %d bo, %s ask == 0 ? "AC" : "BD"
T7S(TX_PDIAG_SLIP1, "That is only half the diagonal. The whole diagonal is made of two such segments.")  // a wrong answer: why
T7S(TX_PDIAG_SLIP2, "You added segments from two different diagonals. The diagonal you need is made of two congruent segments.")  // a wrong answer: why
T7S(TX_PDIAG_SLIP3, "That is the length of the other diagonal. Each diagonal has its own midpoint at O.")  // a wrong answer: why
T7S(TX_PDIAG_SLIP4, "You added all the segments from O to the vertices. The diagonal you need contains only two of them.")  // a wrong answer: why
T7S(TX_PDIAG_HINT1, "What does the theorem say about the point where the diagonals of a parallelogram meet?")  // hint 1
T7S(TX_PDIAG_HINT2, "Point O is the midpoint of each diagonal, so a diagonal is twice as long as one of its halves.")  // hint 2
T7S(TX_PDIAG_SOL, "The diagonals of a parallelogram bisect each other, so O is the midpoint of %s: %s = 2 × %s = 2 × %d = %d cm.")  // the worked solution; %s ask == 0 ? "AC" : "BD", %s ask == 0 ? "AC" : "BD", %s ask == 0 ? "AO" : "BO", %d mine, %d ans

// ---- 4.5 / review: which figure do these properties guarantee? (K_CLASSIFY)
T7S(TX_CLS_LABEL_QUAD, "Only a convex quadrilateral")  // an answer
T7S(TX_CLS_LABEL_PAR, "Parallelogram")  // an answer
T7S(TX_CLS_LABEL_RECT, "Rectangle")  // an answer
T7S(TX_CLS_LABEL_RHOMBUS, "Rhombus")  // an answer
T7S(TX_CLS_LABEL_SQUARE, "Square")  // an answer
T7S(TX_CLS_WHY_NOT_PAR, "The given properties are not enough: some convex quadrilaterals have them and are not parallelograms.")  // a wrong answer: why
T7S(TX_CLS_WHY_NOT_RECT, "The given properties are not enough: some convex quadrilaterals have them and are not rectangles.")  // a wrong answer: why
T7S(TX_CLS_WHY_NOT_RHOMBUS, "The given properties are not enough: some convex quadrilaterals have them and are not rhombuses.")  // a wrong answer: why
T7S(TX_CLS_WHY_NOT_SQUARE, "The given properties are not enough: some convex quadrilaterals have them and are not squares.")  // a wrong answer: why
T7S(TX_CLS_P_DIAG_CONG_BISECT, "congruent diagonals that bisect each other")  // a property in the question's list
T7S(TX_CLS_P_DIAG_PERP_BISECT, "perpendicular diagonals that bisect each other")  // a property in the question's list
T7S(TX_CLS_P_DIAG_BISECT, "diagonals that bisect each other")  // a property in the question's list
T7S(TX_CLS_P_OPP_SIDES_CONG, "both pairs of opposite sides congruent")  // a property in the question's list
T7S(TX_CLS_P_PAIR_PAR_CONG, "one pair of opposite sides parallel and congruent")  // a property in the question's list
T7S(TX_CLS_P_PAIR_PAR, "one pair of opposite sides parallel")  // a property in the question's list
T7S(TX_CLS_P_ALL_SIDES_CONG, "all sides congruent")  // a property in the question's list
T7S(TX_CLS_P_ADJ_SIDES_CONG, "two adjacent sides congruent")  // a property in the question's list
T7S(TX_CLS_P_THREE_RIGHT, "three right angles")  // a property in the question's list
T7S(TX_CLS_P_TWO_RIGHT, "two right angles")  // a property in the question's list
T7S(TX_CLS_P_ONE_RIGHT, "one right angle")  // a property in the question's list
T7S(TX_CLS_P_DIAG_CONG_PERP, "congruent, perpendicular diagonals")  // a property in the question's list
T7S(TX_CLS_P_DIAG_CONG, "congruent diagonals")  // a property in the question's list
T7S(TX_CLS_P_DIAG_PERP, "perpendicular diagonals")  // a property in the question's list
T7S(TX_CLS_Q_PAR, "Parallelogram ABCD has %s. What is the most precise name these properties guarantee?")  // the question; %s list
T7S(TX_CLS_Q_QUAD, "Convex quadrilateral ABCD has %s. What is the most precise name these properties guarantee?")  // the question; %s list
T7S(TX_CLS_WHY2, "The given properties are enough to recognize a special figure. Think about which type of quadrilateral each one describes.")  // a wrong answer: why
T7S(TX_CLS_WHY3, "That is true, but the given properties tell you more. Look for the most precise name.")  // a wrong answer: why
T7S(TX_CLS_WHY4, "A rectangle with unequal sides has the given properties and is not a square.")  // a wrong answer: why
T7S(TX_CLS_WHY5, "A rhombus with no right angle has the given properties and is not a square.")  // a wrong answer: why
T7S(TX_CLS_WHY6, "Some parallelograms have the given properties and are not squares.")  // a wrong answer: why
T7S(TX_CLS_WHY7, "Some parallelograms have the given properties and are not rectangles.")  // a wrong answer: why
T7S(TX_CLS_WHY8, "Some parallelograms have the given properties and are not rhombuses.")  // a wrong answer: why
T7S(TX_CLS_WHY9, "A rhombus with no right angle has the given properties and is not a rectangle.")  // a wrong answer: why
T7S(TX_CLS_WHY10, "A rectangle with unequal sides has the given properties and is not a rhombus.")  // a wrong answer: why
T7S(TX_CLS_WHY11, "Check again what each property guarantees.")  // a wrong answer: why
T7S(TX_CLS_HINT1, "First decide whether the properties guarantee that opposite sides are parallel. Then see what the others add: angles, diagonals, sides.")  // hint 1
T7S(TX_CLS_HINT2, "Pick only what surely follows from all the properties together, not what might sometimes be true.")  // hint 2
T7S(TX_CLS_SOL_QUAD, "The given properties don't guarantee that both pairs of opposite sides are parallel (some trapezoids have them too), so ABCD may not be a parallelogram: the only name left is convex quadrilateral.")  // the worked solution
T7S(TX_CLS_SOL_PAR, "The given properties guarantee that ABCD is a parallelogram, but nothing more: they give no right angle and no congruent adjacent sides.")  // the worked solution
T7S(TX_CLS_SOL_RECT, "ABCD is a parallelogram, and a parallelogram with one right angle or with congruent diagonals is a rectangle (three right angles also give a rectangle).")  // the worked solution
T7S(TX_CLS_SOL_RHOMBUS, "ABCD is a parallelogram, and a parallelogram with two congruent adjacent sides or with perpendicular diagonals is a rhombus (all sides congruent also gives a rhombus).")  // the worked solution
T7S(TX_CLS_SOL_SQUARE, "ABCD is both a rectangle (one right angle or congruent diagonals) and a rhombus (congruent adjacent sides or perpendicular diagonals), so it is a square.")  // the worked solution

// ---- 4.3: a triangle's midsegment (K_MIDLINE)
T7S(TX_MID_Q1, "In triangle ABC, M is the midpoint of side AB and N is the midpoint of side AC. If BC = %d cm, how long is MN?")  // the question; %d bc
T7S(TX_MID_SLIP1, "The midsegment MN is parallel to BC, but it is not as long: it is shorter than the side it runs along.")  // a wrong answer: why
T7S(TX_MID_SLIP2, "You doubled the side's length. The midsegment is shorter than the side parallel to it.")  // a wrong answer: why
T7S(TX_MID_SLIP3, "You divided by four. The midsegment is half the parallel side, not a quarter of it.")  // a wrong answer: why
T7S(TX_MID_SLIP4, "You divided by three, as for the centroid. Here you are working with the midpoints of two sides, so with the midsegment.")  // a wrong answer: why
T7S(TX_MID_SOL1, "MN is a midsegment of triangle ABC, so MN = BC ÷ 2 = %d ÷ 2 = %d cm.")  // the worked solution; %d bc, %d ans
T7S(TX_MID_Q2, "In triangle ABC, M is the midpoint of side AB and N is the midpoint of side AC. If MN = %d cm, how long is BC?")  // the question; %d mn
T7S(TX_MID_SLIP5, "Side BC is longer than the midsegment MN, which is parallel to it.")  // a wrong answer: why
T7S(TX_MID_SLIP6, "You halved instead of doubling. The midsegment is half of BC, so BC is longer than MN.")  // a wrong answer: why
T7S(TX_MID_SLIP7, "You multiplied by three, as for the centroid. Here you have a midsegment, which is half the parallel side.")  // a wrong answer: why
T7S(TX_MID_SLIP8, "You multiplied by four. The midsegment is half the parallel side, not a quarter of it.")  // a wrong answer: why
T7S(TX_MID_SOL2, "MN is a midsegment, so MN = BC ÷ 2. Then BC = 2 × MN = 2 × %d = %d cm.")  // the worked solution; %d mn, %d ans
T7S(TX_MID_HINT1, "Think of the midsegment theorem: the midsegment is parallel to the third side, and its length is simply linked to that side's length.")  // hint 1
T7S(TX_MID_HINT2, "A midsegment is half as long as the side parallel to it. Decide whether to halve or to double, based on what you know.")  // hint 2

// ---- 4.3: the medial triangle (K_MEDIAN_TRIANGLE)
T7S(TX_MEDT_Q, "Triangle ABC has sides AB = %d cm, BC = %d cm and AC = %d cm. Joining the midpoints of the sides gives the medial triangle, formed by the three midsegments. What is its perimeter?")  // the question; %d a, %d b, %d cc
T7S(TX_MEDT_SLIP1, "That is the perimeter of triangle ABC. The midsegments are shorter than the sides.")  // a wrong answer: why
T7S(TX_MEDT_SLIP2, "You divided the perimeter by four. Each midsegment is half of its side, so the perimeter is halved.")  // a wrong answer: why
T7S(TX_MEDT_SLIP3, "You divided by three. Each midsegment is half the side parallel to it.")  // a wrong answer: why
T7S(TX_MEDT_SLIP4, "That is just one midsegment. The perimeter adds all three midsegments.")  // a wrong answer: why
T7S(TX_MEDT_SLIP5, "You doubled the perimeter. The midsegments are shorter than the sides they are parallel to.")  // a wrong answer: why
T7S(TX_MEDT_HINT1, "Each side of the medial triangle is a midsegment. How is it related to the parallel side of triangle ABC?")  // hint 1
T7S(TX_MEDT_HINT2, "The three midsegments are halves of the three sides. Add them, or halve the perimeter.")  // hint 2
T7S(TX_MEDT_SOL, "Each midsegment is half the side parallel to it, so the perimeter of the medial triangle is half the perimeter of ABC: (%d + %d + %d) ÷ 2 = %d ÷ 2 = %d cm.")  // the worked solution; %d a, %d b, %d cc, %d P, %d ans

// ---- 4.3: the centroid divides a median 2 : 1 (K_CENTROID)
T7S(TX_CEN_Q1, "In triangle ABC, AM is a median and G is the centroid. If AM = %d cm, how long is %s?")  // the question; %d given, %s mode == 0 ? "GA" : "GM"
T7S(TX_CEN_SLIP1, "That is the length of GM. The centroid is closer to the midpoint of the side than to the vertex, so GA is the longer segment.")  // a wrong answer: why
T7S(TX_CEN_SLIP2, "You took G to be the midpoint of the median. The centroid does not cut the median in half.")  // a wrong answer: why
T7S(TX_CEN_SLIP3, "That is the length of the whole median AM, and GA is only part of it.")  // a wrong answer: why
T7S(TX_CEN_SLIP4, "That is longer than the median. GA is part of AM, so it is shorter than AM.")  // a wrong answer: why
T7S(TX_CEN_SOL1, "The centroid divides the median in the ratio 2 : 1, starting from the vertex: GA = (2/3) × AM = (2/3) × %d = %d cm.")  // the worked solution; %d given, %d ans
T7S(TX_CEN_SLIP5, "That is the length of GA. Segment GM, on the side of the midpoint, is the shorter of the two.")  // a wrong answer: why
T7S(TX_CEN_SLIP6, "That is the length of the whole median AM, and GM is only part of it.")  // a wrong answer: why
T7S(TX_CEN_SLIP7, "You added a number to a third of the median, which no rule of the theorem says. GM is exactly one part of the median.")  // a wrong answer: why
T7S(TX_CEN_SOL2, "The centroid divides the median in the ratio 2 : 1 from the vertex, so GM is a third of the median: GM = (1/3) × AM = (1/3) × %d = %d cm.")  // the worked solution; %d given, %d ans
T7S(TX_CEN_Q2, "In triangle ABC, AM is a median and G is the centroid. If GM = %d cm, how long is %s?")  // the question; %d given, %s mode == 2 ? "AM" : "GA"
T7S(TX_CEN_SLIP8, "That is the length of GA. The median AM also includes segment GM.")  // a wrong answer: why
T7S(TX_CEN_SLIP9, "That is only the length of GM, part of the median. The whole median is longer.")  // a wrong answer: why
T7S(TX_CEN_SLIP10, "You counted too many parts. The median has exactly three equal parts: two for GA and one for GM.")  // a wrong answer: why
T7S(TX_CEN_SLIP11, "You added a number to GM, which no rule of the theorem says. The median is three parts, each equal to GM.")  // a wrong answer: why
T7S(TX_CEN_SOL3, "GM is a third of the median, so AM = 3 × GM = 3 × %d = %d cm.")  // the worked solution; %d given, %d ans
T7S(TX_CEN_SLIP12, "That is the length of the whole median AM, not of GA. GA is shorter than AM.")  // a wrong answer: why
T7S(TX_CEN_SLIP13, "That is the length of GM. GA is the segment on the vertex side, longer than GM.")  // a wrong answer: why
T7S(TX_CEN_SLIP14, "You halved the length of GM. GA is longer than GM, not shorter.")  // a wrong answer: why
T7S(TX_CEN_SLIP15, "You added a number to GM, which no rule of the theorem says. GA is a multiple of GM.")  // a wrong answer: why
T7S(TX_CEN_SOL4, "GA is twice GM (G divides the median in the ratio 2 : 1 from the vertex): GA = 2 × GM = 2 × %d = %d cm.")  // the worked solution; %d given, %d ans
T7S(TX_CEN_HINT1, "The centroid splits each median into two segments. Which one is longer: the one on the vertex side or the one on the midpoint side?")  // hint 1
T7S(TX_CEN_HINT2, "The median splits into three equal parts: one segment has one part, the other has two parts.")  // hint 2

// ---- 4.4: a rectangle's diagonals (K_RECT_DIAGONAL)
T7S(TX_RDIAG_Q1, "ABCD is a rectangle, and O is the point where its diagonals meet. If AC = %d cm, how long is BD?")  // the question; %d x
T7S(TX_RDIAG_SLIP1, "That is the length of OB, half a diagonal. BD is a whole diagonal; compare it with AC: the diagonals of a rectangle have a special property.")  // a wrong answer: why
T7S(TX_RDIAG_SLIP2, "You doubled a diagonal's length. Each diagonal is given from vertex to vertex, so there is nothing to double.")  // a wrong answer: why
T7S(TX_RDIAG_SLIP3, "That value doesn't follow from any property of the rectangle. Its diagonals are congruent.")  // a wrong answer: why
T7S(TX_RDIAG_SLIP4, "That is much too small. BD is a whole diagonal, not a quarter of the other one.")  // a wrong answer: why
T7S(TX_RDIAG_SOL1, "The diagonals of a rectangle are congruent, so BD = AC = %d cm.")  // the worked solution; %d x
T7S(TX_RDIAG_Q2, "ABCD is a rectangle, and O is the point where its diagonals meet. If AO = %d cm, how long is BD?")  // the question; %d x
T7S(TX_RDIAG_SLIP5, "The diagonals of a rectangle are congruent and bisect each other, so OB = OA. That is OB, half a diagonal, not BD.")  // a wrong answer: why
T7S(TX_RDIAG_SLIP6, "That is the sum of the two diagonals (AC + BD). You only need BD.")  // a wrong answer: why
T7S(TX_RDIAG_SLIP7, "That value doesn't follow from the properties of a rectangle's diagonals.")  // a wrong answer: why
T7S(TX_RDIAG_SLIP8, "You halved the length of AO. The whole diagonal is longer than its half.")  // a wrong answer: why
T7S(TX_RDIAG_SOL2, "The diagonals of a rectangle are congruent and bisect each other, so AC = 2 × AO = 2 × %d = %d cm. Then BD = AC = %d cm.")  // the worked solution; %d x, %d ans, %d ans
T7S(TX_RDIAG_Q3, "ABCD is a rectangle, and O is the point where its diagonals meet. If AO = %d cm, what is AC + BD?")  // the question; %d x
T7S(TX_RDIAG_SLIP9, "That is the length of one diagonal. You need the sum of both diagonals.")  // a wrong answer: why
T7S(TX_RDIAG_SLIP10, "That is only AO, half a diagonal. You need both whole diagonals.")  // a wrong answer: why
T7S(TX_RDIAG_SLIP11, "You doubled one time too many. Each diagonal is twice AO, and there are two diagonals.")  // a wrong answer: why
T7S(TX_RDIAG_SOL3, "The diagonals of a rectangle are congruent and bisect each other: AC = 2 × AO = %d cm and BD = AC = %d cm. Their sum: AC + BD = 2 × %d = %d cm.")  // the worked solution; %d 2 * x, %d 2 * x, %d 2 * x, %d ans
T7S(TX_RDIAG_HINT1, "Remember the property of a rectangle's diagonals: how do they compare with each other, and how do they cut each other at O?")  // hint 1
T7S(TX_RDIAG_HINT2, "The diagonals of a rectangle are congruent, and each has point O as its midpoint. Use both properties.")  // hint 2

// ---- 4.4: the angle between a rectangle's diagonal and side (K_RECT_ANGLE)
T7S(TX_RANG_Q, "ABCD is a rectangle, and its diagonals meet at O. The acute angle between the diagonals, angle AOB, measures %d°. What is the measure of angle OAB?")  // the question; %d al
T7S(TX_RANG_SLIP1, "You subtracted angle AOB from 90°. Triangle AOB is not a right triangle; its angles have a different sum.")  // a wrong answer: why
T7S(TX_RANG_SLIP2, "That is the angle between the diagonal and side AD, not angle OAB. Together the two angles make the right angle at A.")  // a wrong answer: why
T7S(TX_RANG_SLIP3, "That is the obtuse angle between the diagonals (angle AOD), not a base angle of triangle AOB.")  // a wrong answer: why
T7S(TX_RANG_SLIP4, "That is angle AOB itself, at vertex O. You need angle OAB, a base angle of the isosceles triangle.")  // a wrong answer: why
T7S(TX_RANG_SLIP5, "That value doesn't come from the calculation: in isosceles triangle AOB, the two base angles are equal and complete the sum of 180°.")  // a wrong answer: why
T7S(TX_RANG_HINT1, "The diagonals of a rectangle are congruent and bisect each other, so OA = OB. What kind of triangle is AOB?")  // hint 1
T7S(TX_RANG_HINT2, "In an isosceles triangle the base angles are congruent, and the three angles add up to 180°.")  // hint 2
T7S(TX_RANG_SOL, "OA = OB (the diagonals of a rectangle are congruent and bisect each other), so triangle AOB is isosceles: angle OAB = (180° − %d°) ÷ 2 = %d° ÷ 2 = %d°.")  // the worked solution; %d al, %d 180 - al, %d ans

// ---- 4.5: a rhombus's angles (K_RHOMBUS_ANGLE)
T7S(TX_RHA_Q, "ABCD is a rhombus, and diagonal AC makes a %d° angle with side AB. What is the measure of angle %s?")  // the question; %d x, %s mode == 0 ? "BAD" : "ABC"
T7S(TX_RHA_SLIP1, "That is only the angle between the diagonal and side AB. The diagonal bisects angle BAD, so angle BAD is made of two such angles.")  // a wrong answer: why
T7S(TX_RHA_SLIP2, "That is the measure of angle ABD (between diagonal BD and side AB), not of angle BAD.")  // a wrong answer: why
T7S(TX_RHA_SLIP3, "You subtracted the angle between the diagonal and the side from 180°. The right relation uses the fact that the diagonal is an angle bisector.")  // a wrong answer: why
T7S(TX_RHA_SLIP4, "That is angle ABC, consecutive with angle BAD. You need angle BAD.")  // a wrong answer: why
T7S(TX_RHA_SOL1, "The diagonals of a rhombus bisect its angles, so angle BAD = 2 × angle BAC = 2 × %d° = %d°.")  // the worked solution; %d x, %d ans
T7S(TX_RHA_SLIP5, "That is angle BAD, consecutive with angle ABC. Two consecutive angles of a rhombus are supplementary, not equal.")  // a wrong answer: why
T7S(TX_RHA_SLIP6, "That is only the angle between the diagonal and side AB. First find angle BAD, then use consecutive angles.")  // a wrong answer: why
T7S(TX_RHA_SLIP7, "That is the measure of angle ABD, half of angle ABC. You need the whole angle ABC.")  // a wrong answer: why
T7S(TX_RHA_SLIP8, "You subtracted the angle between the diagonal and the side from 180°. First find angle BAD, then the angle consecutive with it.")  // a wrong answer: why
T7S(TX_RHA_SOL2, "Diagonal AC bisects angle BAD, so angle BAD = 2 × %d° = %d°. Angles BAD and ABC are consecutive, so they are supplementary: angle ABC = 180° − %d° = %d°.")  // the worked solution; %d x, %d 2 * x, %d 2 * x, %d ans
T7S(TX_RHA_HINT1, "In a rhombus, the diagonals bisect the angles. What does that tell you about the angle diagonal AC makes with side AB?")  // hint 1
T7S(TX_RHA_HINT2, "A rhombus is a parallelogram: consecutive angles are supplementary, and an angle bisector splits an angle into two equal parts.")  // hint 2

// ---- 4.5: a rhombus's side or perimeter from its diagonals (K_RHOMBUS_SIDE); a, b half diagonals
T7S(TX_RHS_Q_SIDE, "The diagonals of rhombus ABCD are %d cm and %d cm. How long is a side of the rhombus?")  // the question; %d d1, %d d2
T7S(TX_RHS_Q_PERIMETER, "The diagonals of rhombus ABCD are %d cm and %d cm. What is the perimeter of the rhombus?")  // the question; %d d1, %d d2
T7S(TX_RHS_SLIP1, "You added the halves of the diagonals. In right triangle AOB they are the legs, and the side of the rhombus is the hypotenuse: use the Pythagorean theorem.")  // a wrong answer: why
T7S(TX_RHS_SLIP2, "You used the whole diagonals. The diagonals of a rhombus cross at O, and the legs of right triangle AOB are their halves.")  // a wrong answer: why
T7S(TX_RHS_SLIP3, "That is the area of the rhombus, not its side.")  // a wrong answer: why
T7S(TX_RHS_SLIP4, "You took the longer half-diagonal. The side of the rhombus is the hypotenuse of triangle AOB, so it is longer than either half-diagonal.")  // a wrong answer: why
T7S(TX_RHS_SLIP5, "You forgot to take the square root: the sum of the squares of the legs is the side squared, not the side.")  // a wrong answer: why
T7S(TX_RHS_SLIP6, "That is the length of one side. The perimeter adds the four congruent sides.")  // a wrong answer: why
T7S(TX_RHS_SLIP7, "You counted only two sides. A rhombus has four congruent sides.")  // a wrong answer: why
T7S(TX_RHS_SLIP8, "You worked out twice the sum of the half-diagonals, without the Pythagorean theorem. First find the side, as the hypotenuse of triangle AOB.")  // a wrong answer: why
T7S(TX_RHS_SLIP9, "You used the whole diagonals to find the side. The legs of triangle AOB are their halves.")  // a wrong answer: why
T7S(TX_RHS_SLIP10, "That is the area of the rhombus, not its perimeter.")  // a wrong answer: why
T7S(TX_RHS_HINT1, "The diagonals of a rhombus are perpendicular and bisect each other. Look at triangle AOB: what kind of triangle is it, and which sides are its legs?")  // hint 1
T7S(TX_RHS_HINT2_SIDE, "The legs of triangle AOB are the halves of the diagonals, and the side of the rhombus is the hypotenuse: use the Pythagorean theorem.")  // hint 2
T7S(TX_RHS_HINT2_PERIMETER, "The legs of triangle AOB are the halves of the diagonals, and the side of the rhombus is the hypotenuse: use the Pythagorean theorem, then multiply by the number of sides.")  // hint 2
T7S(TX_RHS_SOL1, "The diagonals are perpendicular and cross at their midpoints: AO = %d cm and BO = %d cm. In right triangle AOB: AB² = %d² + %d² = %d, so the side is %s cm.")  // the worked solution; %d a, %d b, %d a, %d b, %d (int)n, %s f
T7S(TX_RHS_SOL2, "The diagonals are perpendicular and cross at their midpoints: AO = %d cm and BO = %d cm. In triangle AOB: AB² = %d² + %d² = %d, so AB = %s cm. Perimeter: 4 × %s = %s cm.")  // the worked solution; %d a, %d b, %d a, %d b, %d (int)n, %s f, %s f, %s g

// ---- 4.5: a square's side from its diagonal (K_SQUARE_SIDE)
T7S(TX_SQS_Q, "The diagonal of a square is %d cm. How long is the side of the square?")  // the question; %d d
T7S(TX_SQS_SLIP1, "The side of the square is shorter than the diagonal: the diagonal is the hypotenuse of an isosceles right triangle, and the side is one of the legs.")  // a wrong answer: why
T7S(TX_SQS_SLIP2, "You halved the diagonal. By the Pythagorean theorem, the diagonal squared is twice the side squared, not four times as big.")  // a wrong answer: why
T7S(TX_SQS_SLIP3, "That would be the diagonal of a square with a side of %d cm. Here the diagonal is given and you are looking for the side.")  // a wrong answer: why; %d d
T7S(TX_SQS_SLIP4, "You forgot to take the square root: that is the side squared, not the side.")  // a wrong answer: why
T7S(TX_SQS_SLIP5, "You used the formula for an equilateral triangle. Here the right triangle is isosceles, and the diagonal is its hypotenuse.")  // a wrong answer: why
T7S(TX_SQS_HINT1, "The diagonal splits the square into two isosceles right triangles. Which side is the hypotenuse, and which are the legs?")  // hint 1
T7S(TX_SQS_HINT2, "Let s be the side. Write the Pythagorean theorem: the diagonal squared is the sum of the squares of the two legs, which is twice s squared.")  // hint 2
T7S(TX_SQS_SOL, "Let s be the side. By the Pythagorean theorem: %d² = s² + s² = 2 × s², so s² = %d ÷ 2 and s = %d ÷ √2 = %s cm.")  // the worked solution; %d d, %d d * d, %d d, %s f

// ---- 4.6: a trapezoid's angles (K_TRAP_ANGLE)
T7S(TX_TANG_Q_ISO, "ABCD is an isosceles trapezoid with AB and CD parallel, and angle %s measures %d°. What is the measure of angle %s?")  // the question; %s given[mode], %d x, %s ask[mode]
T7S(TX_TANG_Q, "ABCD is a trapezoid with AB and CD parallel, and angle %s measures %d°. What is the measure of angle %s?")  // the question; %s given[mode], %d x, %s ask[mode]
T7S(TX_TANG_SLIP1, "Angle %s and angle %s lie along the same non-parallel side, so they are supplementary, not equal.")  // a wrong answer: why; %s given[mode], %s ask[mode]
T7S(TX_TANG_SLIP2, "You subtracted from 90°. The angles along a non-parallel side have a different sum.")  // a wrong answer: why
T7S(TX_TANG_SLIP3, "You subtracted from 360°, the sum of all four angles. Here you only use the two angles along one non-parallel side.")  // a wrong answer: why
T7S(TX_TANG_SLIP4, "You halved what was left up to 180°. The two angles are supplementary; one is not half of the other.")  // a wrong answer: why
T7S(TX_TANG_HINT1_1, "The angle you need and the given angle lie along the same non-parallel side of the trapezoid. How are such angles related?")  // hint 1
T7S(TX_TANG_HINT2_1, "In any trapezoid, the angles along a non-parallel side are supplementary: they add up to 180°.")  // hint 2
T7S(TX_TANG_SOL1, "AB and CD are parallel, so the angles along the non-parallel side %s are supplementary: angle %s = 180° − %d° = %d°.")  // the worked solution; %s mode == 0 ? "AD" : "BC", %s ask[mode], %d x, %d ans
T7S(TX_TANG_SLIP5, "You thought angle C is congruent to angle A. In an isosceles trapezoid, the angles along the same base are congruent: A with B, and C with D.")  // a wrong answer: why
T7S(TX_TANG_SLIP6, "You subtracted from 90°. There is no such relation in a trapezoid.")  // a wrong answer: why
T7S(TX_TANG_SLIP7, "You subtracted from 360°, the sum of all four angles. You only use two angles.")  // a wrong answer: why
T7S(TX_TANG_SLIP8, "That is the sum of angle C and angle D, not the measure of angle C alone.")  // a wrong answer: why
T7S(TX_TANG_HINT1_2, "First find angle D, which lies along the same non-parallel side AD as angle A. Then use the property of an isosceles trapezoid.")  // hint 1
T7S(TX_TANG_HINT2_2, "Angles A and D are supplementary, and in an isosceles trapezoid the angles along the shorter base are congruent.")  // hint 2
T7S(TX_TANG_SOL2, "Angles A and D lie along the non-parallel side AD, so angle D = 180° − %d° = %d°. In an isosceles trapezoid the angles along the shorter base are congruent: angle C = angle D = %d°.")  // the worked solution; %d x, %d ans, %d ans
T7S(TX_TANG_SLIP9, "That is the measure of the angles along the shorter base. Angle B lies along the same base AB as angle A.")  // a wrong answer: why
T7S(TX_TANG_SLIP10, "You subtracted from 360°, the sum of all four angles. You only use one angle.")  // a wrong answer: why
T7S(TX_TANG_SLIP11, "That is the sum of angles C and D, not the measure of angle B.")  // a wrong answer: why
T7S(TX_TANG_HINT1_3, "Angles A and B lie along the same base of the trapezoid. What property do they have in an isosceles trapezoid?")  // hint 1
T7S(TX_TANG_HINT2_3, "In an isosceles trapezoid, the angles along the same base are congruent.")  // hint 2
T7S(TX_TANG_SOL3, "The trapezoid is isosceles, and angles A and B lie along base AB, so they are congruent: angle B = angle A = %d°.")  // the worked solution; %d x

// ---- 4.6: a trapezoid's midsegment (K_TRAP_MIDLINE); B, b the bases
T7S(TX_TMID_Q1, "Trapezoid ABCD has bases AB = %d cm and CD = %d cm. M and N are the midpoints of the non-parallel sides AD and BC. How long is MN?")  // the question; %d B, %d b
T7S(TX_TMID_SLIP1, "That is the sum of the bases. The midsegment is half of that sum.")  // a wrong answer: why
T7S(TX_TMID_SLIP2, "That is the length of the segment between the diagonals, not the midsegment. The midsegment uses the sum of the bases.")  // a wrong answer: why
T7S(TX_TMID_SLIP3, "You subtracted the bases. The midsegment of a trapezoid uses the sum of the bases.")  // a wrong answer: why
T7S(TX_TMID_SLIP4, "You divided the sum of the bases by four. The midsegment is half the sum of the bases.")  // a wrong answer: why
T7S(TX_TMID_SLIP5, "You took half of the longer base. The midsegment depends on both bases.")  // a wrong answer: why
T7S(TX_TMID_SOL1, "MN is the midsegment of the trapezoid, so MN = (AB + CD) ÷ 2 = (%d + %d) ÷ 2 = %d ÷ 2 = %d cm.")  // the worked solution; %d B, %d b, %d B + b, %d ans
T7S(TX_TMID_Q2, "The midsegment MN of trapezoid ABCD is %d cm, and the shorter base CD is %d cm. How long is the longer base AB?")  // the question; %d m, %d b
T7S(TX_TMID_SLIP6, "You added the midsegment and the shorter base. The midsegment is half the sum of the bases, so the sum of the bases is twice the midsegment.")  // a wrong answer: why
T7S(TX_TMID_SLIP7, "That is the sum of the bases. Subtract the shorter base from it to get the longer base.")  // a wrong answer: why
T7S(TX_TMID_SLIP8, "You subtracted the shorter base from the midsegment. First double the midsegment to get the sum of the bases.")  // a wrong answer: why
T7S(TX_TMID_SLIP9, "You added the shorter base to the sum of the bases. Subtract it from the sum instead.")  // a wrong answer: why
T7S(TX_TMID_SOL2, "MN = (AB + CD) ÷ 2, so AB + CD = 2 × MN = 2 × %d = %d cm. Then AB = %d − %d = %d cm.")  // the worked solution; %d m, %d 2 * m, %d 2 * m, %d b, %d ans
T7S(TX_TMID_Q3, "Trapezoid ABCD has bases AB = %d cm and CD = %d cm. Its midsegment MN crosses the diagonals AC and BD at P and Q. How long is PQ?")  // the question; %d B, %d b
T7S(TX_TMID_SLIP10, "That is the length of the whole midsegment MN. PQ is only its piece between the diagonals.")  // a wrong answer: why
T7S(TX_TMID_SLIP11, "You subtracted the bases, but the segment between the diagonals is half of that difference.")  // a wrong answer: why
T7S(TX_TMID_SLIP12, "That is half the shorter base, which is the length of MP, not PQ.")  // a wrong answer: why
T7S(TX_TMID_SLIP13, "That is half the longer base, which is the length of MQ, not PQ.")  // a wrong answer: why
T7S(TX_TMID_SOL3, "The piece of the midsegment between the diagonals is half the difference of the bases: PQ = (AB − CD) ÷ 2 = (%d − %d) ÷ 2 = %d ÷ 2 = %d cm.")  // the worked solution; %d B, %d b, %d B - b, %d ans
T7S(TX_TMID_HINT1_PQ, "On the midsegment of a trapezoid, the piece between the diagonals has a length linked to the difference of the bases. What is the relation?")  // hint 1
T7S(TX_TMID_HINT1_MN, "The midsegment of a trapezoid joins the midpoints of the non-parallel sides. How is its length linked to the lengths of the bases?")  // hint 1
T7S(TX_TMID_HINT2_PQ, "The segment between the diagonals is half the difference of the bases (the longer base minus the shorter base).")  // hint 2
T7S(TX_TMID_HINT2_MN, "The midsegment is half the sum of the bases. Use the relation in the direction that fits what you know.")  // hint 2

// ---- 4.6: an isosceles trapezoid's height (K_TRAP_HEIGHT); p the piece of the longer base, c0 the side
T7S(TX_THGT_Q, "Isosceles trapezoid ABCD has bases AB = %d cm and CD = %d cm, and non-parallel sides AD = BC = %d cm. What is the height of the trapezoid?")  // the question; %d B, %d b, %d c0
T7S(TX_THGT_SLIP1, "That is the length of the non-parallel side, which is slanted. The height is perpendicular to the bases, so it is shorter than that side.")  // a wrong answer: why
T7S(TX_THGT_SLIP2, "You added the squares. The non-parallel side is the hypotenuse of the right triangle the height makes, so the height squared is a difference.")  // a wrong answer: why
T7S(TX_THGT_SLIP3, "You subtracted the lengths. The Pythagorean theorem works with squares: the height squared is a difference of squares.")  // a wrong answer: why
T7S(TX_THGT_SLIP4, "That is the piece of the longer base under the non-parallel side (its projection), which is half the difference of the bases, not the height.")  // a wrong answer: why
T7S(TX_THGT_SLIP5, "You forgot to take the square root: the difference of the squares is the height squared, not the height.")  // a wrong answer: why
T7S(TX_THGT_SLIP6, "The piece of the longer base under the side is half the difference of the bases, not the whole difference.")  // a wrong answer: why
T7S(TX_THGT_HINT1, "Draw the heights from the ends of the shorter base: they make two congruent right triangles. How long is the piece of the longer base under each non-parallel side?")  // hint 1
T7S(TX_THGT_HINT2, "That piece is half the difference of the bases. Then use the Pythagorean theorem: the non-parallel side is the hypotenuse, and the height is a leg.")  // hint 2
T7S(TX_THGT_SOL, "The piece of the longer base under the side is (AB − CD) ÷ 2 = (%d − %d) ÷ 2 = %d cm. In the right triangle this makes: h² = %d² − %d² = %d, so h = %s cm.")  // the worked solution; %d B, %d b, %d p, %d c0, %d p, %d (int)n, %s f

// ---- 4.7: areas (K_AREA)
T7S(TX_AREA_Q1, "Parallelogram ABCD has base AB = %d cm, side BC = %d cm, and the height to base AB is %d cm. What is its area?")  // the question; %d a, %d s, %d h
T7S(TX_AREA_SLIP1, "You multiplied the base by the slanted side. The area is the base times the height, and the height is perpendicular to the base, not slanted.")  // a wrong answer: why
T7S(TX_AREA_SLIP2, "You divided by two, as for a triangle. A parallelogram's area is the base times the height, with no division.")  // a wrong answer: why
T7S(TX_AREA_SLIP3, "That is the perimeter, not the area.")  // a wrong answer: why
T7S(TX_AREA_SLIP4, "You added the base and the height. The area comes from multiplying.")  // a wrong answer: why
T7S(TX_AREA_HINT1_1, "The area of a parallelogram uses a base and the height to that base. Which of the given numbers are these?")  // hint 1
T7S(TX_AREA_HINT2_1, "Multiply the base by its height. The slanted side is not part of the formula.")  // hint 2
T7S(TX_AREA_SOL1, "A = AB × h = %d × %d = %d cm².")  // the worked solution; %d a, %d h, %d ans
T7S(TX_AREA_Q2, "A triangle has a side of %d cm, and the height to that side is %d cm. What is the area of the triangle?")  // the question; %d b, %d h
T7S(TX_AREA_SLIP5, "You forgot to divide by two. A triangle's area is half the product of a side and its height.")  // a wrong answer: why
T7S(TX_AREA_SLIP6, "You added the side and the height. The area comes from multiplying, then dividing by two.")  // a wrong answer: why
T7S(TX_AREA_SLIP7, "You divided by two twice. The formula divides by two only once.")  // a wrong answer: why
T7S(TX_AREA_SLIP8, "You took the average of the side and the height. A triangle's area is half their product.")  // a wrong answer: why
T7S(TX_AREA_HINT1_2, "A triangle's area is linked to the area of a rectangle or parallelogram with the same base and height. What is the link?")  // hint 1
T7S(TX_AREA_HINT2_2, "Multiply the side by its height, then divide the result by two.")  // hint 2
T7S(TX_AREA_SOL2, "A = (side × height) ÷ 2 = (%d × %d) ÷ 2 = %d ÷ 2 = %d cm².")  // the worked solution; %d b, %d h, %d b * h, %d ans
T7S(TX_AREA_Q3, "A rhombus has diagonals of %d cm and %d cm. What is its area?")  // the question; %d d1, %d d2
T7S(TX_AREA_SLIP9, "You forgot to divide by two. The area of a rhombus is half the product of its diagonals.")  // a wrong answer: why
T7S(TX_AREA_SLIP10, "You added the diagonals. The area comes from multiplying, then dividing by two.")  // a wrong answer: why
T7S(TX_AREA_SLIP11, "You divided by four. The area of a rhombus is half the product of its diagonals, not a quarter.")  // a wrong answer: why
T7S(TX_AREA_SLIP12, "You took the average of the diagonals. The area is half their product.")  // a wrong answer: why
T7S(TX_AREA_HINT1_3, "The diagonals split the rhombus into four congruent right triangles. What are the legs of each one?")  // hint 1
T7S(TX_AREA_HINT2_3, "The area of a rhombus is half the product of its diagonals.")  // hint 2
T7S(TX_AREA_SOL3, "A = (d1 × d2) ÷ 2 = (%d × %d) ÷ 2 = %d ÷ 2 = %d cm².")  // the worked solution; %d d1, %d d2, %d d1 * d2, %d ans
T7S(TX_AREA_Q4, "Trapezoid ABCD has longer base AB = %d cm, shorter base CD = %d cm and height %d cm. What is its area?")  // the question; %d B, %d b, %d h
T7S(TX_AREA_SLIP13, "You forgot to divide by two. The area of a trapezoid is half the sum of the bases times the height.")  // a wrong answer: why
T7S(TX_AREA_SLIP14, "You used only the longer base. The area of a trapezoid depends on both bases.")  // a wrong answer: why
T7S(TX_AREA_SLIP15, "You subtracted the bases. The area formula uses the sum of the bases.")  // a wrong answer: why
T7S(TX_AREA_SLIP16, "That is the length of the midsegment. For the area, multiply it by the height.")  // a wrong answer: why
T7S(TX_AREA_HINT1_4, "A trapezoid's area can be found as the midsegment times the height. How do you get the midsegment from the bases?")  // hint 1
T7S(TX_AREA_HINT2_4, "Add the bases, multiply the sum by the height, then divide by two.")  // hint 2
T7S(TX_AREA_SOL4, "A = ((AB + CD) × h) ÷ 2 = ((%d + %d) × %d) ÷ 2 = %d ÷ 2 = %d cm².")  // the worked solution; %d B, %d b, %d h, %d (B + b) * h, %d ans
T7S(TX_AREA_Q5, "Triangle ABC has angle A = 90°, leg AB = %d cm and leg AC = %d cm. The hypotenuse BC is %d cm. What is the area of the triangle?")  // the question; %d c1, %d c2, %d hy
T7S(TX_AREA_SLIP17, "You forgot to divide by two. The area of a right triangle is half the product of the legs.")  // a wrong answer: why
T7S(TX_AREA_SLIP18, "You added the legs. The area comes from multiplying, then dividing by two.")  // a wrong answer: why
T7S(TX_AREA_SLIP19, "You multiplied the hypotenuse by a leg. The height to the hypotenuse is not a leg; the legs are perpendicular to each other, so each one is the height to the other.")  // a wrong answer: why
T7S(TX_AREA_SLIP20, "That is the perimeter of the triangle (the sum of the sides), not its area.")  // a wrong answer: why
T7S(TX_AREA_HINT1_5, "In a right triangle, each leg is the height to the other leg. What does that mean for the area?")  // hint 1
T7S(TX_AREA_HINT2_5, "Multiply the two legs, then divide by two. You don't need the hypotenuse.")  // hint 2
T7S(TX_AREA_SOL5, "The legs are perpendicular, so A = (AB × AC) ÷ 2 = (%d × %d) ÷ 2 = %d ÷ 2 = %d cm².")  // the worked solution; %d c1, %d c2, %d c1 * c2, %d ans

// ---- 4.7: perimeters (K_PERIMETER)
T7S(TX_PER_Q1, "A rectangle has a length of %d cm and a width of %d cm. What is its perimeter?")  // the question; %d a, %d b
T7S(TX_PER_Q2, "A parallelogram has two adjacent sides of %d cm and %d cm. What is its perimeter?")  // the question; %d a, %d b
T7S(TX_PER_SLIP1, "That is the sum of two adjacent sides. The perimeter adds all four sides.")  // a wrong answer: why
T7S(TX_PER_SLIP2, "That is the product of the lengths (the area of a rectangle with these sides), not the perimeter.")  // a wrong answer: why
T7S(TX_PER_SLIP3, "You counted one side only once. Opposite sides are congruent, so each length appears twice.")  // a wrong answer: why
T7S(TX_PER_HINT1_1, "The perimeter is the sum of the lengths of all the sides. How many sides are there, and which ones are congruent?")  // hint 1
T7S(TX_PER_HINT2_1, "The perimeter is twice the sum of two adjacent sides.")  // hint 2
T7S(TX_PER_SOL1, "P = 2 × (%d + %d) = 2 × %d = %d cm.")  // the worked solution; %d a, %d b, %d a + b, %d ans
T7S(TX_PER_Q_SQUARE, "A square has a side of %d cm. What is its perimeter?")  // the question; %d l
T7S(TX_PER_Q_RHOMBUS, "A rhombus has a side of %d cm. What is its perimeter?")  // the question; %d l
T7S(TX_PER_SLIP4, "That is the area of a square with this side, not the perimeter.")  // a wrong answer: why
T7S(TX_PER_SLIP5, "You counted only two sides. The figure has four congruent sides.")  // a wrong answer: why
T7S(TX_PER_SLIP6, "You counted only three sides. The figure has four congruent sides.")  // a wrong answer: why
T7S(TX_PER_SLIP7, "You counted each side twice. The figure has only four sides.")  // a wrong answer: why
T7S(TX_PER_HINT1_2, "All the sides of the figure are congruent. How many sides does it have?")  // hint 1
T7S(TX_PER_HINT2_2, "The perimeter is the sum of the four congruent sides.")  // hint 2
T7S(TX_PER_SOL2, "P = 4 × s = 4 × %d = %d cm.")  // the worked solution; %d l, %d ans
T7S(TX_DM_HALF, "%d.5")  // a number; %d y / 10
T7S(TX_PER_Q4, "A rectangle has a length of %d cm and a width of %s dm. What is its perimeter, in centimeters?")  // the question; %d a, %s dm
T7S(TX_PER_SLIP8, "You added centimeters and decimeters without converting the units. First write the width in centimeters (1 dm = 10 cm).")  // a wrong answer: why
T7S(TX_PER_SLIP9, "That is the product of the lengths, not the perimeter.")  // a wrong answer: why
T7S(TX_PER_SLIP10, "You counted the width only once. Opposite sides are congruent, so each length appears twice.")  // a wrong answer: why
T7S(TX_PER_HINT1_3, "The two sides are not in the same unit. What do you do before calculating?")  // hint 1
T7S(TX_PER_HINT2_3, "Change the decimeters into centimeters, then take twice the sum of the two adjacent sides.")  // hint 2
T7S(TX_PER_SOL3, "Width: %s dm = %d cm. P = 2 × (%d + %d) = 2 × %d = %d cm.")  // the worked solution; %s dm, %d y, %d a, %d y, %d a + y, %d ans

// ---- 4.7: a height from the area (K_HEIGHT_FROM_AREA)
T7S(TX_HFA_Q1, "The area of a parallelogram is %d cm², and one of its sides is %d cm. What is the height to that side?")  // the question; %d A, %d a
T7S(TX_HFA_SLIP1, "You subtracted the side from the area. The area is the side times the height, so you find the height by dividing.")  // a wrong answer: why
T7S(TX_HFA_SLIP2, "You multiplied the area by the side. From A = side × height, you get the height by dividing the area by the side.")  // a wrong answer: why
T7S(TX_HFA_SLIP3, "You also divided by two, as for a triangle. A parallelogram's area is the base times the height, with no division.")  // a wrong answer: why
T7S(TX_HFA_SLIP4, "You multiplied by two, as for a triangle. A parallelogram's area is the base times the height, with no other factors.")  // a wrong answer: why
T7S(TX_HFA_HINT1_1, "The area of a parallelogram is a side times the height to that side. Which factor is missing?")  // hint 1
T7S(TX_HFA_HINT2_1, "Divide the area by the given side.")  // hint 2
T7S(TX_HFA_SOL1, "A = side × h, so h = A ÷ side = %d ÷ %d = %d cm.")  // the worked solution; %d A, %d a, %d h
T7S(TX_HFA_Q2, "The area of a triangle is %d cm², and one of its sides is %d cm. What is the height to that side?")  // the question; %d A, %d b
T7S(TX_HFA_SLIP5, "You divided the area by the side, but a triangle's area is half the side times the height, so the height is bigger.")  // a wrong answer: why
T7S(TX_HFA_SLIP6, "You subtracted the side from the area. The area is half the product of the side and the height.")  // a wrong answer: why
T7S(TX_HFA_SLIP7, "You multiplied the area by the side. You find the height by dividing, not multiplying.")  // a wrong answer: why
T7S(TX_HFA_SLIP8, "You multiplied by four. From A = (side × height) ÷ 2, the height is twice the area divided by the side.")  // a wrong answer: why
T7S(TX_HFA_HINT1_2, "A triangle's area is half the product of a side and the height to it. How do you get the height from this relation?")  // hint 1
T7S(TX_HFA_HINT2_2, "Double the area, then divide by the given side.")  // hint 2
T7S(TX_HFA_SOL2, "A = (side × h) ÷ 2, so h = 2 × A ÷ side = 2 × %d ÷ %d = %d cm.")  // the worked solution; %d A, %d b, %d h
T7S(TX_HFA_Q3, "Triangle ABC has angle A = 90°, leg AB = %d cm, leg AC = %d cm and hypotenuse BC = %d cm. What is the height from A to the hypotenuse?")  // the question; %d c1, %d c2, %d hy
T7S(TX_HFA_SLIP9, "That is the area of the triangle, not the height.")  // a wrong answer: why
T7S(TX_HFA_SLIP10, "That is the length of the median from A (half the hypotenuse), not the height.")  // a wrong answer: why
T7S(TX_HFA_SLIP11, "That is the product of the legs. You get the height by dividing this product by the hypotenuse.")  // a wrong answer: why
T7S(TX_HFA_SLIP12, "You divided the product of the legs by their sum. The height to the hypotenuse comes from dividing by the hypotenuse.")  // a wrong answer: why
T7S(TX_HFA_SLIP13, "You took the average of the legs, which is not the height to the hypotenuse.")  // a wrong answer: why
T7S(TX_HFA_HINT1_3, "You can find the area of a right triangle in two ways: with the legs, or with the hypotenuse and the height to it. Set them equal.")  // hint 1
T7S(TX_HFA_HINT2_3, "The area with the legs is half their product. The area with the hypotenuse is half the hypotenuse times the height.")  // hint 2
T7S(TX_HFA_SOL3, "Write the area in two ways: (AB × AC) ÷ 2 = (BC × h) ÷ 2, so h = (AB × AC) ÷ BC = (%d × %d) ÷ %d = %d ÷ %d = %s cm.")  // the worked solution; %d c1, %d c2, %d hy, %d c1 * c2, %d hy, %s f
