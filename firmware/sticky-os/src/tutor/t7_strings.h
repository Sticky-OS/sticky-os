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

// ---- The unit list (the textbook's units, in its order)
T7S(TX_UNIT_4, "Unit 4: The quadrilateral")  // a button, 12pt, up to 2 lines
T7S(TX_UNIT_5, "Unit 5: The circle")  // a button
T7S(TX_UNIT_6, "Unit 6: Similar triangles")  // a button
T7S(TX_UNIT_7, "Unit 7: The right triangle")  // a button

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

// (Tutor, Unit 5, the circle: English rows, T7S(TX_ID, "text") one per line. Merged into t7_strings.h at the end.)
// Answers with pi are written "12π cm" (the screen draws π). Questions that want pi say "in terms of π".

// ---- Arcs and central angles (K_ARC_CENTRAL)
T7S(TX_ARC_Q_MINOR, "In a circle with centre O, the central angle AOB measures %d°. What is the measure of the minor arc AB?")  // the question; %d the angle
T7S(TX_ARC_SLIP_HALF, "You halved the angle. Halving belongs to an inscribed angle. A central angle is as big as its minor arc.")  // a wrong answer: why
T7S(TX_ARC_SLIP_DOUBLE, "You doubled the angle. A central angle is as big as the minor arc between its sides, no more.")  // a wrong answer: why
T7S(TX_ARC_SLIP_MAJOR, "You gave the major arc, the rest of the circle. The minor arc is as big as the central angle.")  // a wrong answer: why
T7S(TX_ARC_SLIP_SUPP, "You subtracted the angle from 180°. The arcs of a whole circle add up to 360°, and a central angle is as big as its minor arc.")  // a wrong answer: why
T7S(TX_ARC_HINT1, "A central angle has its vertex at the centre O. How do its measure and the measure of the arc between its sides compare? And what does the whole circle measure?")  // hint 1
T7S(TX_ARC_HINT2_MINOR, "A central angle and the minor arc between its sides have the same measure.")  // hint 2
T7S(TX_ARC_SOL_MINOR, "A central angle has the same measure as its minor arc: arc AB = %d°.")  // the worked solution; %d the angle
T7S(TX_ARC_Q_MAJOR, "In a circle with centre O, the central angle AOB measures %d°. What is the measure of the major arc AB?")  // the question; %d the angle
T7S(TX_ARC_SLIP_MINOR, "You gave the minor arc. The major arc is the rest of the circle.")  // a wrong answer: why
T7S(TX_ARC_SLIP_TWICE, "You subtracted the angle twice. Take it away from the whole circle just once.")  // a wrong answer: why
T7S(TX_ARC_HINT2_MAJOR, "The minor arc and the major arc together make the whole circle, 360°. Find the minor arc first.")  // hint 2
T7S(TX_ARC_SOL_MAJOR, "The minor arc AB measures %d°. The major arc is the rest of the circle: 360° − %d° = %d°.")  // the worked solution; %d the angle, %d the angle, %d the answer
T7S(TX_ARC_Q_REST, "Points A, B, C and D divide a circle into four arcs. Arc AB = %d°, arc BC = %d° and arc CD = %d°. What is the measure of arc DA?")  // the question; %d arc AB, %d arc BC, %d arc CD
T7S(TX_ARC_SLIP_SUM, "That is the sum of the three given arcs. The missing arc is what is left to complete the whole circle.")  // a wrong answer: why
T7S(TX_ARC_SLIP_180, "You compared the sum of the arcs with 180°. A whole circle is 360°, and the four arcs add up to 360°.")  // a wrong answer: why
T7S(TX_ARC_SLIP_TWO, "You took only two of the three given arcs away from 360°. All three given arcs must be taken away.")  // a wrong answer: why
T7S(TX_ARC_HINT1_REST, "The four arcs together make the whole circle. What do their measures add up to?")  // hint 1
T7S(TX_ARC_HINT2_REST, "Add the three given arcs and take the sum away from the measure of the whole circle.")  // hint 2
T7S(TX_ARC_SOL_REST, "The arcs add up to 360°. Arc AB + arc BC + arc CD = %d° + %d° + %d° = %d°, so arc DA = 360° − %d° = %d°.")  // the worked solution; %d AB, %d BC, %d CD, %d their sum, %d the sum, %d the answer
T7S(TX_ARC_Q_RATIO, "Points A, B and C divide a circle into three arcs AB, BC and CA. Their measures are proportional to the numbers %d, %d and %d. What is the measure of arc %s?")  // the question; %d, %d, %d the numbers for AB, BC, CA, %s the arc asked
T7S(TX_ARC_SLIP_EQUAL, "You split the circle into three equal arcs. The arcs are not equal: they are proportional to the given numbers.")  // a wrong answer: why
T7S(TX_ARC_SLIP_PART, "That is the measure of one part. The arc is made of several such parts.")  // a wrong answer: why
T7S(TX_ARC_SLIP_REST, "That is the measure of the other two arcs together, not of the arc asked.")  // a wrong answer: why
T7S(TX_ARC_SLIP_OTHER, "That is the measure of a different arc. Check which arc the question asks for.")  // a wrong answer: why
T7S(TX_ARC_SLIP_NUMBER, "You gave the number from the ratio. The arc is that number of parts, and each part is worth some degrees.")  // a wrong answer: why
T7S(TX_ARC_HINT1_RATIO, "Think of the three arcs as made of equal parts. How many parts fill the whole circle?")  // hint 1
T7S(TX_ARC_HINT2_RATIO, "Divide the whole circle by the total number of parts to find one part, then multiply by the number of the arc asked.")  // hint 2
T7S(TX_ARC_SOL_RATIO, "The circle is 360°. Number of parts: %d + %d + %d = %d, so one part is 360° ÷ %d = %d°. Arc %s: %d × %d° = %d°.")  // the worked solution; %d, %d, %d the ratio, %d the total, %d the total, %d one part, %s the arc, %d its number, %d one part, %d the answer
T7S(TX_ARC_Q_SEMI, "AB is a diameter of a circle. A point C lies on one of the two semicircles, so that arc AC measures %d°. What is the measure of arc CB on that same semicircle?")  // the question; %d arc AC
T7S(TX_ARC_SLIP_360, "You subtracted from 360°. Both arcs lie on the same semicircle, which is only half of the circle.")  // a wrong answer: why
T7S(TX_ARC_SLIP_SAME, "That is the measure of arc AC itself. Arc CB is the rest of the semicircle.")  // a wrong answer: why
T7S(TX_ARC_SLIP_90, "You subtracted from 90°. A semicircle is half of the circle, more than a quarter of it.")  // a wrong answer: why
T7S(TX_ARC_SLIP_HALFX, "You halved the given arc. The missing arc is what is left of the semicircle after taking the given arc away.")  // a wrong answer: why
T7S(TX_ARC_SLIP_PLUS, "You added the given arc to the semicircle. The two arcs on the same semicircle fill it exactly.")  // a wrong answer: why
T7S(TX_ARC_HINT1_SEMI, "A diameter cuts the circle into two semicircles. How many degrees does a semicircle measure?")  // hint 1
T7S(TX_ARC_HINT2_SEMI, "The two arcs AC and CB together make the whole semicircle.")  // hint 2
T7S(TX_ARC_SOL_SEMI, "A semicircle is 180°. Arc CB = 180° − arc AC = 180° − %d° = %d°.")  // the worked solution; %d arc AC, %d the answer

// ---- A chord and its distance from the centre (K_CHORD_DISTANCE)
T7S(TX_CHD_Q_CHORD, "In a circle with centre O and radius %d cm, the chord AB is at a distance of %d cm from the centre. How long is the chord AB?")  // the question; %d radius, %d distance
T7S(TX_CHD_SLIP_HALF, "You found only half of the chord, AM. The perpendicular from the centre bisects the chord, so AB is twice AM.")  // a wrong answer: why
T7S(TX_CHD_SLIP_DIFF, "You subtracted lengths instead of squares. Pythagoras's theorem works with squares: AM² = OA² − OM².")  // a wrong answer: why
T7S(TX_CHD_SLIP_ADD, "You added the squares. OA is the hypotenuse of the right triangle OMA, so a square must be taken away: AM² = OA² − OM².")  // a wrong answer: why
T7S(TX_CHD_SLIP_NOROOT, "You forgot the square root. The subtraction gives AM², not a length.")  // a wrong answer: why
T7S(TX_CHD_SLIP_DIAM, "You gave the diameter. A chord that does not go through the centre is shorter than the diameter.")  // a wrong answer: why
T7S(TX_CHD_HINT1, "Draw OM from the centre, perpendicular to the chord AB; M is the midpoint of AB. Which right triangle do you see?")  // hint 1
T7S(TX_CHD_HINT2_CHORD, "In the right triangle OMA the hypotenuse is the radius OA. Find the leg AM with Pythagoras's theorem, then think how AB is related to AM.")  // hint 2
T7S(TX_CHD_SOL_CHORD, "M is the midpoint of AB and OM is perpendicular to AB. In triangle OMA: AM² = OA² − OM² = %d² − %d² = %lld − %lld = %lld, so AM = %s cm and AB = 2 × AM = %s cm.")  // the worked solution; %d r, %d d, %lld r², %lld d², %lld the difference, %s the root, %s the chord
T7S(TX_CHD_Q_DIST, "In a circle with centre O and radius %d cm, a chord AB is %d cm long. At what distance from the centre O is the chord?")  // the question; %d radius, %d chord
T7S(TX_CHD_SLIP_D_HALF, "That is half of the chord, AM. The distance from the centre is OM, the other leg of triangle OMA.")  // a wrong answer: why
T7S(TX_CHD_SLIP_D_DIFF, "You subtracted lengths: the radius minus half the chord. Use Pythagoras's theorem with squares: OM² = OA² − AM².")  // a wrong answer: why
T7S(TX_CHD_SLIP_D_ADD, "You added the squares. OA is the hypotenuse of the right triangle OMA, so OM² = OA² − AM².")  // a wrong answer: why
T7S(TX_CHD_SLIP_D_NOROOT, "You forgot the square root. The subtraction gives OM², not a length.")  // a wrong answer: why
T7S(TX_CHD_SLIP_D_WHOLE, "You used the whole chord as a leg. The leg of triangle OMA is AM, half of the chord.")  // a wrong answer: why
T7S(TX_CHD_HINT2_DIST, "AM is half of the chord, and the radius OA is the hypotenuse of the right triangle OMA. Find the other leg, OM.")  // hint 2
T7S(TX_CHD_SOL_DIST, "M is the midpoint of AB, so AM = %d ÷ 2 = %d cm. In the right triangle OMA: OM² = OA² − AM² = %d² − %d² = %lld − %lld = %lld, so OM = %s cm.")  // the worked solution; %d chord, %d half chord, %d r, %d half chord, %lld r², %lld half², %lld the difference, %s the root
T7S(TX_CHD_Q_RADIUS, "A chord AB of a circle with centre O is %d cm long, and its distance from the centre is %d cm. What is the radius of the circle?")  // the question; %d chord, %d distance
T7S(TX_CHD_SLIP_R_SUM, "You added the distance and half of the chord. The radius is the hypotenuse of the right triangle OMA, so use squares: OA² = OM² + AM².")  // a wrong answer: why
T7S(TX_CHD_SLIP_R_NOROOT, "You forgot the square root. The sum of the squares gives OA², not a length.")  // a wrong answer: why
T7S(TX_CHD_SLIP_R_WHOLE, "You used the whole chord as a leg. The leg of triangle OMA is AM, half of the chord.")  // a wrong answer: why
T7S(TX_CHD_SLIP_R_HALF, "You took half of the chord as the radius. The radius is the hypotenuse of triangle OMA, longer than the leg AM.")  // a wrong answer: why
T7S(TX_CHD_SLIP_R_DIFF, "You subtracted the squares. The radius is the hypotenuse, so the squares of the legs are added.")  // a wrong answer: why
T7S(TX_CHD_HINT2_RADIUS, "AM is half of the chord and OM is the distance: they are the legs of the right triangle OMA. The radius OA is its hypotenuse.")  // hint 2
T7S(TX_CHD_SOL_RADIUS, "M is the midpoint of AB, so AM = %d ÷ 2 = %d cm. In the right triangle OMA: OA² = OM² + AM² = %d² + %d² = %lld + %lld = %lld, so OA = %s cm.")  // the worked solution; %d chord, %d half chord, %d d, %d half chord, %lld d², %lld half², %lld the sum, %s the root

// ---- The inscribed angle (K_INSCRIBED)
T7S(TX_INS_Q_ANGLE, "In a circle, the arc BC (the one that does not contain A) measures %d°. What is the measure of the inscribed angle BAC?")  // the question; %d the arc
T7S(TX_INS_SLIP_FULL, "You gave the measure of the arc. An inscribed angle is only half of the arc it looks at.")  // a wrong answer: why
T7S(TX_INS_SLIP_DBL, "You doubled the arc. An inscribed angle is half of its arc, not twice.")  // a wrong answer: why
T7S(TX_INS_SLIP_SUPP, "You subtracted half of the arc from 180°. That is for the angle on the other side of the chord. Here the angle is simply half of its arc.")  // a wrong answer: why
T7S(TX_INS_SLIP_QUARTER, "You halved twice. An inscribed angle is half of its arc, once.")  // a wrong answer: why
T7S(TX_INS_HINT1, "An inscribed angle has its vertex on the circle. It is measured by the arc between its sides, the arc that does not contain the vertex.")  // hint 1
T7S(TX_INS_HINT2_ANGLE, "The inscribed angle is half of the arc it looks at.")  // hint 2
T7S(TX_INS_SOL_ANGLE, "An inscribed angle is half of its arc: %d° ÷ 2 = %d°.")  // the worked solution; %d the arc, %d the answer
T7S(TX_INS_Q_ARC, "In a circle, the inscribed angle BAC measures %d°. What is the measure of the arc BC that does not contain A?")  // the question; %d the angle
T7S(TX_INS_SLIP_SAME, "You gave the measure of the angle. The arc it looks at is twice as big as the inscribed angle.")  // a wrong answer: why
T7S(TX_INS_SLIP_OTHERARC, "That is the other arc, the one that contains the vertex A. The angle looks at the arc between its sides, away from its vertex.")  // a wrong answer: why
T7S(TX_INS_SLIP_HALFANG, "You halved the angle. It is the arc that is twice the inscribed angle.")  // a wrong answer: why
T7S(TX_INS_SLIP_180Y, "You subtracted the angle from 180°. That relation is for opposite angles of an inscribed quadrilateral, not for an angle and its arc.")  // a wrong answer: why
T7S(TX_INS_SLIP_FOUR, "You doubled twice. The arc is twice the inscribed angle, once.")  // a wrong answer: why
T7S(TX_INS_HINT2_ARC, "The arc is twice the inscribed angle that looks at it.")  // hint 2
T7S(TX_INS_SOL_ARC, "The arc is twice the inscribed angle: 2 × %d° = %d°.")  // the worked solution; %d the angle, %d the answer
T7S(TX_INS_Q_DIAM, "BC is a diameter of a circle and A is another point of the circle. What is the measure of the angle BAC?")  // the question
T7S(TX_INS_SLIP_D180, "That is the measure of the arc on the diameter, a semicircle. The angle is half of its arc.")  // a wrong answer: why
T7S(TX_INS_SLIP_D45, "You halved twice. The angle is half of the semicircle once.")  // a wrong answer: why
T7S(TX_INS_SLIP_D60, "An angle on a diameter does not depend on where A is, and it is not 60°. It is half of the semicircle.")  // a wrong answer: why
T7S(TX_INS_SLIP_D360, "You took the whole circle. The arc of a diameter is only a semicircle, and the angle is half of that arc.")  // a wrong answer: why
T7S(TX_INS_HINT1_DIAM, "A diameter cuts the circle into two semicircles. The angle BAC looks at one of them.")  // hint 1
T7S(TX_INS_HINT2_DIAM, "How many degrees does a semicircle measure? The inscribed angle is half of its arc.")  // hint 2
T7S(TX_INS_SOL_DIAM, "BC is a diameter, so the arc BC is a semicircle, 180°. The inscribed angle BAC is half of it: 180° ÷ 2 = 90°. An angle inscribed in a semicircle is a right angle.")  // the worked solution
T7S(TX_INS_Q_DIAMTRI, "BC is a diameter of a circle and A is another point of the circle. The angle ABC measures %d°. What is the measure of the angle ACB?")  // the question; %d angle ABC
T7S(TX_INS_SLIP_T180, "You subtracted from 180° but forgot that triangle ABC has a right angle at A.")  // a wrong answer: why
T7S(TX_INS_SLIP_TADD, "You added the angle to 90°. The three angles of the triangle add up to 180°, so the angle is taken away.")  // a wrong answer: why
T7S(TX_INS_SLIP_TSAME, "You gave the angle ABC. Nothing says the triangle is isosceles.")  // a wrong answer: why
T7S(TX_INS_SLIP_TTWICE, "You took the angle away twice. The triangle has one such angle at B and the right angle at A.")  // a wrong answer: why
T7S(TX_INS_HINT2_DIAMTRI, "The angle BAC looks at the diameter, so find it first. Then use the sum of the angles of triangle ABC.")  // hint 2
T7S(TX_INS_SOL_DIAMTRI, "The angle BAC looks at the diameter BC, so it is 90°. In triangle ABC: angle ACB = 180° − 90° − %d° = %d°.")  // the worked solution; %d angle ABC, %d the answer
T7S(TX_INS_Q_THREE, "The vertices of triangle ABC lie on a circle. Arc AB = %d° and arc CA = %d° (each arc does not contain the third vertex). What is the measure of the angle BAC?")  // the question; %d arc AB, %d arc CA
T7S(TX_INS_SLIP_GIVEN, "You halved the sum of the two given arcs. The angle BAC looks at the third arc, BC, between the other two vertices.")  // a wrong answer: why
T7S(TX_INS_SLIP_ADJ, "You halved one of the given arcs. Those arcs touch the vertex A, but the angle BAC looks at the arc BC, opposite A.")  // a wrong answer: why
T7S(TX_INS_HINT2_THREE, "The three arcs fill the circle. Find the arc BC opposite A, then halve it.")  // hint 2
T7S(TX_INS_SOL_THREE, "The arcs fill the circle: arc BC = 360° − %d° − %d° = %d°. The angle BAC looks at arc BC: %d° ÷ 2 = %d°.")  // the worked solution; %d arc AB, %d arc CA, %d arc BC, %d arc BC, %d the answer
T7S(TX_INS_Q_TRI, "Triangle ABC is inscribed in a circle. The arc BC that does not contain A measures %d°, and the angle ABC measures %d°. What is the measure of the angle ACB?")  // the question; %d arc BC, %d angle ABC
T7S(TX_INS_SLIP_WHOLEARC, "You took away the whole arc instead of half of it. The angle BAC is half of the arc BC.")  // a wrong answer: why
T7S(TX_INS_SLIP_NOA, "You left out the angle at A. The three angles of the triangle add up to 180°.")  // a wrong answer: why
T7S(TX_INS_SLIP_NOREST, "You added the angles at A and B. The angle at C is what is left of the 180° of the triangle.")  // a wrong answer: why
T7S(TX_INS_SLIP_NOB, "You left out the angle at B. The angle at C is what is left after taking away both the angle at A and the angle at B.")  // a wrong answer: why
T7S(TX_INS_HINT2_TRI, "Find the angle BAC from its arc first. Then use the sum of the angles of triangle ABC.")  // hint 2
T7S(TX_INS_SOL_TRI, "The angle BAC looks at the arc BC: %d° ÷ 2 = %d°. In triangle ABC: angle ACB = 180° − %d° − %d° = %d°.")  // the worked solution; %d arc BC, %d the angle BAC, %d the angle BAC, %d angle ABC, %d the answer
T7S(TX_INS_Q_OTHER, "B and C are points on a circle and A is a point on the major arc BC, with the angle BAC = %d°. D is a point on the minor arc BC, on the other side of the chord BC from A. What is the measure of the angle BDC?")  // the question; %d the angle BAC
T7S(TX_INS_SLIP_O_SAME, "You gave the same angle. Equal angles stand on the same side of the chord. D is on the other side.")  // a wrong answer: why
T7S(TX_INS_SLIP_O_360, "You subtracted from 360°. The two angles on the two sides of a chord add up to a half-turn, not a full turn.")  // a wrong answer: why
T7S(TX_INS_SLIP_O_90, "You subtracted from 90°. The two angles on the two sides of a chord add up to a straight angle.")  // a wrong answer: why
T7S(TX_INS_SLIP_O_ARC, "That is the arc BC seen from A, not the angle at D. Remember: an inscribed angle is half of its arc.")  // a wrong answer: why
T7S(TX_INS_HINT2_OTHER, "A and D look at the two different arcs of the chord BC, which together fill the circle. What do the two angles add up to?")  // hint 2
T7S(TX_INS_SOL_OTHER, "A and D are on different sides of the chord BC, so the angles BAC and BDC add up to 180°: angle BDC = 180° − %d° = %d°.")  // the worked solution; %d angle BAC, %d the answer
T7S(TX_INS_Q_MINOR, "The minor arc BC of a circle measures %d°, and A is a point on this minor arc. What is the measure of the angle BAC?")  // the question; %d the minor arc
T7S(TX_INS_SLIP_M_HALF, "You halved the minor arc. The angle BAC looks at the arc between its sides that does not contain A, and A is on the minor arc.")  // a wrong answer: why
T7S(TX_INS_SLIP_M_WHOLE, "You found the major arc, but forgot to halve it. An inscribed angle is half of its arc.")  // a wrong answer: why
T7S(TX_INS_SLIP_M_180, "You subtracted the minor arc from 180°. Take the major arc, the one that does not contain A, and halve it.")  // a wrong answer: why
T7S(TX_INS_HINT2_MINOR, "Which arc does the angle BAC look at, the minor or the major one? It is the arc that does not contain the vertex A.")  // hint 2
T7S(TX_INS_SOL_MINOR, "A is on the minor arc, so the angle BAC looks at the major arc: 360° − %d° = %d°. The angle is half of it: %d° ÷ 2 = %d°.")  // the worked solution; %d the minor arc, %d the major arc, %d the major arc, %d the answer

// ---- A quadrilateral inscribed in a circle (K_CYCLIC_QUAD)
T7S(TX_CYC_Q_OPP, "Quadrilateral ABCD is inscribed in a circle. The angle %s measures %d°. What is the measure of the angle %s?")  // the question; %s the given vertex, %d its angle, %s the vertex asked
T7S(TX_CYC_SLIP_SAME, "You gave the same angle. In an inscribed quadrilateral, opposite angles are supplementary, not equal.")  // a wrong answer: why
T7S(TX_CYC_SLIP_360, "You subtracted from 360°. That is the sum of all four angles. Two opposite angles add up to half of it.")  // a wrong answer: why
T7S(TX_CYC_SLIP_90, "You compared the angle with 90°. Opposite angles of an inscribed quadrilateral add up to a straight angle.")  // a wrong answer: why
T7S(TX_CYC_SLIP_DBL, "You doubled the angle. That gives the arc it looks at. The question asks for the opposite angle.")  // a wrong answer: why
T7S(TX_CYC_HINT1, "ABCD is inscribed in a circle, so there is a rule for its opposite angles. What do they add up to?")  // hint 1
T7S(TX_CYC_HINT2_OPP, "Opposite angles of an inscribed quadrilateral are supplementary: take the given angle away from a straight angle.")  // hint 2
T7S(TX_CYC_SOL_OPP, "Opposite angles of an inscribed quadrilateral add up to 180°: angle %s = 180° − angle %s = 180° − %d° = %d°.")  // the worked solution; %s the asked vertex, %s the given vertex, %d the given angle, %d the answer
T7S(TX_CYC_Q_ARCS, "Quadrilateral ABCD is inscribed in a circle. Arc %s = %d° and arc %s = %d° (the two arcs that do not contain the vertex %s). What is the measure of the angle %s?")  // the question; %s first arc, %d its measure, %s second arc, %d its measure, %s the vertex, %s the vertex
T7S(TX_CYC_SLIP_ARC, "You added the two arcs but did not halve them. An inscribed angle is half of the arc it looks at.")  // a wrong answer: why
T7S(TX_CYC_SLIP_OPPOSITE, "That is the opposite angle. The angle asked looks at the two arcs given in the question.")  // a wrong answer: why
T7S(TX_CYC_SLIP_ONE, "You used only one of the two arcs. The angle looks at both of them together.")  // a wrong answer: why
T7S(TX_CYC_HINT1_ARCS, "An inscribed angle is half of the arc it looks at. Which arcs does the angle asked look at?")  // hint 1
T7S(TX_CYC_HINT2_ARCS, "The angle looks at the arcs that do not contain its vertex. Add them, then halve the sum.")  // hint 2
T7S(TX_CYC_SOL_ARCS, "The angle %s looks at the arcs %s and %s, together %d° + %d° = %d°. An inscribed angle is half of its arc: %d° ÷ 2 = %d°.")  // the worked solution; %s the vertex, %s first arc, %s second arc, %d, %d the arcs, %d their sum, %d the sum, %d the answer
T7S(TX_CYC_Q_TWO, "Quadrilateral ABCD is inscribed in a circle. The angle %s measures %d° and the angle %s measures %d°. What is the measure of the angle %s?")  // the question; %s vertex, %d its angle, %s vertex, %d its angle, %s the vertex asked
T7S(TX_CYC_SLIP_BOTH, "You took both given angles away from 360°. That gives the sum of the two unknown angles, not one of them.")  // a wrong answer: why
T7S(TX_CYC_SLIP_WRONGPAIR, "You used the other given angle. The angle asked is opposite to one of the given angles, and only that one counts.")  // a wrong answer: why
T7S(TX_CYC_SLIP_BOTH180, "You took both angles away from 180°. Opposite angles go in pairs: use only the one opposite to the angle asked.")  // a wrong answer: why
T7S(TX_CYC_SLIP_OTHERGIVEN, "You gave one of the angles from the question. The angle asked is supplementary to the angle opposite it.")  // a wrong answer: why
T7S(TX_CYC_HINT1_TWO, "Two of the angles are given. Which given angle is opposite to the angle asked?")  // hint 1
T7S(TX_CYC_HINT2_TWO, "Opposite angles of an inscribed quadrilateral are supplementary. The other given angle is not needed.")  // hint 2
T7S(TX_CYC_SOL_TWO, "The angle %s is opposite to the angle %s, so they add up to 180°: angle %s = 180° − %d° = %d°.")  // the worked solution; %s asked vertex, %s the opposite vertex, ...
T7S(TX_CYC_Q_THREE, "Quadrilateral ABCD is inscribed in a circle. Arc AB = %d°, arc BC = %d° and arc CD = %d°. What is the measure of the angle %s?")  // the question; %d arc AB, %d arc BC, %d arc CD, %s the vertex asked
T7S(TX_CYC_SLIP_ALLTHREE, "You halved the sum of the three given arcs. The angle looks only at the two arcs that do not contain its vertex.")  // a wrong answer: why
T7S(TX_CYC_SLIP_TWICEHALF, "You halved twice. An inscribed angle is half of its arc, once.")  // a wrong answer: why
T7S(TX_CYC_HINT2_THREE, "The four arcs fill the circle, so the fourth arc is known too. Then add the two arcs the angle looks at and halve the sum.")  // hint 2
T7S(TX_CYC_SOL_THREE, "The arcs add up to 360°, so arc DA = 360° − (%d° + %d° + %d°) = 360° − %d° = %d°. The angle %s looks at the arcs %d° and %d°, together %d°: half of it is %d°.")  // the worked solution; %d AB, %d BC, %d CD, %d their sum, %d arc DA, %s the vertex, %d, %d the two arcs, %d their sum, %d the answer

// ---- The tangent from an outside point (K_TANGENT_LENGTH)
T7S(TX_TAN_Q_AC, "A is a point outside the circle with centre O and radius %d cm, and AC is the tangent segment from A (C is the point of tangency). OA = %d cm. How long is AC?")  // the question; %d radius, %d OA
T7S(TX_TAN_SLIP_DIFF, "You subtracted lengths. Pythagoras's theorem works with squares: AC² = OA² − OC².")  // a wrong answer: why
T7S(TX_TAN_SLIP_ADD, "You added the squares. OA is the hypotenuse of the right triangle OCA, so AC² = OA² − OC².")  // a wrong answer: why
T7S(TX_TAN_SLIP_NOROOT, "You forgot the square root. The subtraction gives AC², not a length.")  // a wrong answer: why
T7S(TX_TAN_SLIP_HYP, "You gave OA, the hypotenuse. The tangent segment AC is a leg, shorter than OA.")  // a wrong answer: why
T7S(TX_TAN_HINT1, "The radius OC meets the tangent AC at the point of tangency C. What angle do they make? What kind of triangle is OCA?")  // hint 1
T7S(TX_TAN_HINT2_AC, "The hypotenuse of the right triangle OCA is OA. Find the leg AC with Pythagoras's theorem.")  // hint 2
T7S(TX_TAN_SOL_AC, "The radius OC is perpendicular to the tangent AC, so triangle OCA has a right angle at C. AC² = OA² − OC² = %d² − %d² = %lld − %lld = %lld, so AC = %s cm.")  // the worked solution; %d OA, %d r, %lld OA², %lld r², %lld the difference, %s the root
T7S(TX_TAN_Q_OA, "A is a point outside the circle with centre O and radius %d cm, and AC is the tangent segment from A (C is the point of tangency). AC = %d cm. How far is A from the centre O?")  // the question; %d radius, %d AC
T7S(TX_TAN_SLIP_SUM, "You added the lengths. Pythagoras's theorem adds the squares of the legs: OA² = OC² + AC².")  // a wrong answer: why
T7S(TX_TAN_SLIP_NOROOT2, "You forgot the square root. The sum of the squares gives OA², not a length.")  // a wrong answer: why
T7S(TX_TAN_SLIP_WRONGSUB, "You subtracted the squares. OA is the hypotenuse, so the squares of the legs are added.")  // a wrong answer: why
T7S(TX_TAN_SLIP_LONGERLEG, "You gave the longer of the two legs. The hypotenuse OA is longer than both legs.")  // a wrong answer: why
T7S(TX_TAN_HINT2_OA, "OC and AC are the legs of the right triangle OCA, and OA is its hypotenuse.")  // hint 2
T7S(TX_TAN_SOL_OA, "The radius OC is perpendicular to the tangent AC, so triangle OCA has a right angle at C. OA² = OC² + AC² = %d² + %d² = %lld + %lld = %lld, so OA = %s cm.")  // the worked solution; %d r, %d AC, %lld r², %lld AC², %lld the sum, %s the root
T7S(TX_TAN_Q_R, "A is a point outside a circle with centre O, and OA = %d cm. The tangent from A touches the circle at C and AC = %d cm. What is the radius of the circle?")  // the question; %d OA, %d AC
T7S(TX_TAN_SLIP_DIFF2, "You subtracted lengths. Pythagoras's theorem works with squares: OC² = OA² − AC².")  // a wrong answer: why
T7S(TX_TAN_SLIP_NOROOT3, "You forgot the square root. The subtraction gives OC², not a length.")  // a wrong answer: why
T7S(TX_TAN_SLIP_ADD2, "You added the squares. OA is the hypotenuse of the right triangle OCA, so OC² = OA² − AC².")  // a wrong answer: why
T7S(TX_TAN_SLIP_TANASR, "You gave the tangent segment, which was given. The radius is the other leg of triangle OCA.")  // a wrong answer: why
T7S(TX_TAN_HINT2_R, "The radius OC and the tangent segment AC are the legs of the right triangle OCA. OA is the hypotenuse.")  // hint 2
T7S(TX_TAN_SOL_R, "The radius OC is perpendicular to the tangent AC, so triangle OCA has a right angle at C. OC² = OA² − AC² = %d² − %d² = %lld − %lld = %lld, so the radius is %s cm.")  // the worked solution; %d OA, %d AC, %lld OA², %lld AC², %lld the difference, %s the root

// ---- A quadrilateral round a circle (K_CIRCUMSCRIBED)
T7S(TX_CIR_Q_SIDE, "A circle is inscribed in quadrilateral ABCD (all four sides touch the circle). %s = %d cm, %s = %d cm and %s = %d cm. How long is the side %s?")  // the question; %s side, %d its length (three times), %s the side asked
T7S(TX_CIR_SLIP_SUM, "You added the three given sides. The rule compares the sums of opposite sides.")  // a wrong answer: why
T7S(TX_CIR_SLIP_NEIGHBOURS, "You compared sums of neighbouring sides. The rule is about opposite sides: AB + CD = BC + DA.")  // a wrong answer: why
T7S(TX_CIR_SLIP_OPPEQUAL, "You made the opposite side equal. That is a property of a parallelogram. For a quadrilateral round a circle, the sums of opposite sides are equal.")  // a wrong answer: why
T7S(TX_CIR_HINT1, "A quadrilateral whose sides touch a circle has a rule about the lengths of its opposite sides.")  // hint 1
T7S(TX_CIR_HINT2_SIDE, "The two sums of opposite sides are equal. Which side is opposite to the side asked, and which two sides make the other pair?")  // hint 2
T7S(TX_CIR_SOL_SIDE, "The sums of opposite sides are equal: %s + %s = %s + %s. The known pair: %d + %d = %d. So %s = %d − %d = %d cm.")  // the worked solution; %s four side names, %d, %d the known pair, %d their sum, %s the side asked, %d the sum, %d the opposite side, %d the answer
T7S(TX_CIR_Q_PERIM, "A circle is inscribed in quadrilateral ABCD (all four sides touch the circle). %s = %d cm, %s = %d cm and %s = %d cm. What is the perimeter of ABCD?")  // the question; %s side, %d its length (three times)
T7S(TX_CIR_SLIP_PSUM, "You added only the three given sides. The perimeter includes the fourth side too.")  // a wrong answer: why
T7S(TX_CIR_SLIP_PHALF, "That is the sum of one pair of opposite sides, half of the perimeter. Add the other pair too.")  // a wrong answer: why
T7S(TX_CIR_SLIP_POPP, "You added two neighbouring sides and doubled. The rule is about opposite sides, which have equal sums.")  // a wrong answer: why
T7S(TX_CIR_SLIP_PFOUR, "You made all four sides equal to one of them. They are not equal. Use the sums of opposite sides.")  // a wrong answer: why
T7S(TX_CIR_HINT2_PERIM, "The two sums of opposite sides are equal, and the perimeter is both sums together. Find a pair of opposite sides you know.")  // hint 2
T7S(TX_CIR_SOL_PERIM, "The sums of opposite sides are equal, and the perimeter is both sums together. %s + %s = %d + %d = %d, so the perimeter is 2 × %d = %d cm.")  // the worked solution; %s, %s the opposite sides, %d, %d their lengths, %d their sum, %d the sum, %d the answer

// ---- The area of a triangle and its inscribed circle (K_INCIRCLE_AREA)
T7S(TX_INC_Q_R, "A triangle has area %d cm² and perimeter %d cm. What is the radius of its inscribed circle?")  // the question; %d area, %d perimeter
T7S(TX_INC_SLIP_R_MULT, "You multiplied the area by the semiperimeter. From A = p × r the radius is found by dividing.")  // a wrong answer: why
T7S(TX_INC_SLIP_R_SEMI, "That is the semiperimeter, not the radius.")  // a wrong answer: why
T7S(TX_INC_SLIP_R_DIAM, "That is twice the radius, the diameter of the inscribed circle.")  // a wrong answer: why
T7S(TX_INC_SLIP_R_AREA, "That is the area of the triangle, not the radius.")  // a wrong answer: why
T7S(TX_INC_SLIP_R_PERIM, "You divided by the whole perimeter. The formula uses the semiperimeter, half of the perimeter.")  // a wrong answer: why
T7S(TX_INC_SLIP_R_SUB, "You subtracted the semiperimeter from the area. In A = p × r the two are multiplied, so r is found by dividing.")  // a wrong answer: why
T7S(TX_INC_HINT1, "The area of a triangle can be found from its semiperimeter p, half of the perimeter, and the radius r of its inscribed circle.")  // hint 1
T7S(TX_INC_HINT2_R, "The area is the semiperimeter times the radius, so the radius is the area divided by the semiperimeter.")  // hint 2
T7S(TX_INC_SOL_R, "The semiperimeter is p = %d ÷ 2 = %d cm. From A = p × r: r = A ÷ p = %d ÷ %d = %d cm.")  // the worked solution; %d perimeter, %d semiperimeter, %d area, %d semiperimeter, %d the answer
T7S(TX_INC_Q_AREA, "A circle of radius %d cm is inscribed in a triangle with sides %d cm, %d cm and %d cm. What is the area of the triangle?")  // the question; %d radius, %d, %d, %d the sides
T7S(TX_INC_Q_AREA_P, "A circle of radius %d cm is inscribed in a triangle with perimeter %d cm. What is the area of the triangle?")  // the question; %d radius, %d perimeter
T7S(TX_INC_SLIP_A_PERIM, "You multiplied the radius by the whole perimeter. The formula uses the semiperimeter, half of the perimeter.")  // a wrong answer: why
T7S(TX_INC_SLIP_A_ADD, "You added the semiperimeter and the radius. In A = p × r they are multiplied.")  // a wrong answer: why
T7S(TX_INC_SLIP_A_ADDP, "You added the perimeter and the radius. The area is the semiperimeter times the radius.")  // a wrong answer: why
T7S(TX_INC_SLIP_A_TWICE, "You halved twice: once for the semiperimeter and again as if the formula had a half in front of it. A = p × r has no other half.")  // a wrong answer: why
T7S(TX_INC_HINT2_AREA, "The area is the product of the semiperimeter and the radius of the inscribed circle. Find the semiperimeter first.")  // hint 2
T7S(TX_INC_SOL_AREA, "The perimeter is %d + %d + %d = %d cm, so the semiperimeter is p = %d ÷ 2 = %d cm. Area = p × r = %d × %d = %d cm².")  // the worked solution; %d, %d, %d the sides, %d perimeter, %d perimeter, %d semiperimeter, %d semiperimeter, %d radius, %d the answer
T7S(TX_INC_SOL_AREA_P, "The semiperimeter is p = %d ÷ 2 = %d cm. Area = p × r = %d × %d = %d cm².")  // the worked solution; %d perimeter, %d semiperimeter, %d semiperimeter, %d radius, %d the answer

// ---- Regular polygons (K_REGULAR_POLYGON)
T7S(TX_RP_Q_CENTRAL, "A regular polygon with %d sides is inscribed in a circle with centre O. What is the measure of the central angle that corresponds to one side?")  // the question; %d the number of sides
T7S(TX_RP_SLIP_C_POLY, "That is the angle of the polygon, at a vertex. The central angle is at the centre O.")  // a wrong answer: why
T7S(TX_RP_SLIP_C_HALF, "You divided 180° by the number of sides. The central angles fill the whole turn around O, 360°.")  // a wrong answer: why
T7S(TX_RP_SLIP_C_NM2, "You divided 360° by the number of sides minus 2. Divide by the number of sides itself.")  // a wrong answer: why
T7S(TX_RP_SLIP_C_N, "That is the number of sides, not an angle in degrees.")  // a wrong answer: why
T7S(TX_RP_HINT1_C, "The central angles of a regular polygon are all equal, and together they fill the full turn around the centre.")  // hint 1
T7S(TX_RP_HINT2_C, "Divide the full turn by the number of sides.")  // hint 2
T7S(TX_RP_SOL_C, "The central angles of the polygon are equal and together make 360°: 360° ÷ %d = %d°.")  // the worked solution; %d the number of sides, %d the answer
T7S(TX_RP_Q_ANGLE, "A regular polygon with %d sides is inscribed in a circle. What is the measure of one angle of the polygon?")  // the question; %d the number of sides
T7S(TX_RP_SLIP_A_CENTRAL, "That is the central angle, at the centre of the circle. The angle of the polygon is at a vertex.")  // a wrong answer: why
T7S(TX_RP_SLIP_A_SUM, "That is the sum of all the angles. Divide it by the number of angles.")  // a wrong answer: why
T7S(TX_RP_SLIP_A_NM1, "You used the number of sides minus 1 instead of minus 2 in the sum of the angles.")  // a wrong answer: why
T7S(TX_RP_SLIP_A_360, "You used 360° instead of 180° in the sum (n − 2) × 180°.")  // a wrong answer: why
T7S(TX_RP_HINT1_A, "The angles of a regular polygon are all equal. How much is the sum of all of them?")  // hint 1
T7S(TX_RP_HINT2_A, "The sum of the angles of a polygon with n sides is (n − 2) × 180°. Divide it by the number of angles.")  // hint 2
T7S(TX_RP_SOL_A, "The sum of the angles is (%d − 2) × 180° = %d°. The polygon has %d equal angles: %d° ÷ %d = %d°.")  // the worked solution; %d n, %d the sum, %d n, %d the sum, %d n, %d the answer
T7S(TX_RP_Q_SUM, "A regular polygon has %d sides. What is the sum of the measures of its angles?")  // the question; %d the number of sides
T7S(TX_RP_SLIP_S_360, "You used 360° instead of 180° in (n − 2) × 180°.")  // a wrong answer: why
T7S(TX_RP_SLIP_S_N, "You multiplied 180° by the number of sides. The polygon splits into n − 2 triangles, not n.")  // a wrong answer: why
T7S(TX_RP_SLIP_S_NM1, "You used the number of sides minus 1 instead of minus 2. From one vertex you get n − 2 triangles.")  // a wrong answer: why
T7S(TX_RP_SLIP_S_ONE, "That is one angle of the polygon. The question asks for the sum of all of them.")  // a wrong answer: why
T7S(TX_RP_HINT1_S, "From one vertex, the polygon can be cut into triangles. How many triangles do you get for n sides?")  // hint 1
T7S(TX_RP_HINT2_S, "You get n − 2 triangles, and the angles of each triangle add up to a straight angle.")  // hint 2
T7S(TX_RP_SOL_S, "The sum of the angles of a polygon with n sides is (n − 2) × 180°: (%d − 2) × 180° = %d × 180° = %d°.")  // the worked solution; %d n, %d n − 2, %d the answer
T7S(TX_RP_Q_N_CENTRAL, "The central angle that corresponds to one side of a regular polygon inscribed in a circle measures %d°. How many sides does the polygon have?")  // the question; %d the central angle
T7S(TX_RP_SLIP_N_HALF, "You divided 180° by the central angle. The central angles fill the full turn, 360°.")  // a wrong answer: why
T7S(TX_RP_SLIP_N_NM2, "That is two less than the number of sides. The number of sides is the full turn divided by the central angle.")  // a wrong answer: why
T7S(TX_RP_SLIP_N_NP2, "That is two more than the number of sides. The number of sides is the full turn divided by the central angle.")  // a wrong answer: why
T7S(TX_RP_SLIP_N_NM1, "That is one less than the number of sides. The number of sides is the full turn divided by the central angle.")  // a wrong answer: why
T7S(TX_RP_HINT1_NC, "The central angles of a regular polygon are all equal and fill the full turn around the centre.")  // hint 1
T7S(TX_RP_HINT2_NC, "How many angles of this size fit into a full turn?")  // hint 2
T7S(TX_RP_SOL_NC, "The central angles fill 360°, so n = 360° ÷ %d° = %d.")  // the worked solution; %d the central angle, %d the answer
T7S(TX_RP_Q_N_ANGLE, "Each angle of a regular polygon measures %d°. How many sides does the polygon have?")  // the question; %d the angle
T7S(TX_RP_SLIP_N_CENTRAL, "That is the angle outside the polygon at a vertex, which is also the central angle. It is not the number of sides.")  // a wrong answer: why
T7S(TX_RP_SLIP_NA_NM2, "That is two less than the number of sides. Put it into (n − 2) × 180 = n × the given angle and check.")  // a wrong answer: why
T7S(TX_RP_SLIP_NA_NP2, "That is two more than the number of sides. Put it into (n − 2) × 180 = n × the given angle and check.")  // a wrong answer: why
T7S(TX_RP_SLIP_NA_NM1, "That is one less than the number of sides. Put it into (n − 2) × 180 = n × the given angle and check.")  // a wrong answer: why
T7S(TX_RP_HINT1_NA, "Write the angle of a regular polygon with n sides, (n − 2) × 180° ÷ n, and set it equal to the given angle.")  // hint 1
T7S(TX_RP_HINT2_NA, "Solve the equation: (n − 2) × 180 = n × the given angle.")  // hint 2
T7S(TX_RP_SOL_NA, "(n − 2) × 180 = n × %d, so 180n − 360 = %dn. Then (180 − %d) × n = 360, and n = 360 ÷ %d = %d.")  // the worked solution; %d the angle (three times), %d 180 minus the angle, %d the answer
T7S(TX_RP_Q_HEXPER, "A regular hexagon is inscribed in a circle of radius %d cm. What is the perimeter of the hexagon?")  // the question; %d radius
T7S(TX_RP_SLIP_H_DIAM, "You used the diameter as the side. The side of a regular hexagon equals the radius.")  // a wrong answer: why
T7S(TX_RP_SLIP_H_FIVE, "You counted five sides. A hexagon has six sides.")  // a wrong answer: why
T7S(TX_RP_SLIP_H_ONE, "That is the length of one side. The perimeter is the sum of all six sides.")  // a wrong answer: why
T7S(TX_RP_SLIP_H_THREE, "That is the length of three sides, half of the perimeter. A hexagon has six sides.")  // a wrong answer: why
T7S(TX_RP_HINT1_HEX, "In a regular hexagon inscribed in a circle, the triangle made by the centre and one side is equilateral. What does that say about the side?")  // hint 1
T7S(TX_RP_HINT2_HEXPER, "The side equals the radius, and the hexagon has six equal sides.")  // hint 2
T7S(TX_RP_SOL_HEXPER, "The triangle made by the centre and a side is equilateral, so the side equals the radius, %d cm. Perimeter: 6 × %d = %d cm.")  // the worked solution; %d radius, %d radius, %d the answer
T7S(TX_RP_Q_HEXR, "A regular hexagon has perimeter %d cm. What is the radius of the circle through its vertices?")  // the question; %d perimeter
T7S(TX_RP_SLIP_HR_12, "You divided by 12. The side of the hexagon equals the radius, not the diameter.")  // a wrong answer: why
T7S(TX_RP_SLIP_HR_3, "You divided by 3. The hexagon has six sides.")  // a wrong answer: why
T7S(TX_RP_SLIP_HR_2, "You halved the perimeter. That is the length of three sides, not one.")  // a wrong answer: why
T7S(TX_RP_SLIP_HR_P, "That is the perimeter itself. Share it among the six equal sides: each side is the radius.")  // a wrong answer: why
T7S(TX_RP_HINT2_HEXR, "The side equals the radius. The perimeter is made of six sides.")  // hint 2
T7S(TX_RP_SOL_HEXR, "The side equals the radius r, and the perimeter is six sides: 6 × r = %d, so r = %d ÷ 6 = %d cm.")  // the worked solution; %d perimeter, %d perimeter, %d the answer
T7S(TX_RP_Q_AREA_PA, "A regular polygon has perimeter %d cm and apothem %d cm (the apothem is the distance from the centre to a side). What is its area?")  // the question; %d perimeter, %d apothem
T7S(TX_RP_SLIP_P_FULL, "You forgot the half. The area of a regular polygon is half of the product of the perimeter and the apothem.")  // a wrong answer: why
T7S(TX_RP_SLIP_P_ADD, "You added the perimeter and the apothem. They are multiplied, and the product is halved.")  // a wrong answer: why
T7S(TX_RP_SLIP_P_HALFP, "That is half of the perimeter. The area also needs the apothem.")  // a wrong answer: why
T7S(TX_RP_SLIP_P_QUARTER, "You halved twice. The area is the perimeter times the apothem, divided by 2 once.")  // a wrong answer: why
T7S(TX_RP_HINT1_AREA, "A regular polygon splits into equal triangles with a vertex at the centre. The area has a formula with the perimeter and the apothem.")  // hint 1
T7S(TX_RP_HINT2_AREA, "Multiply the perimeter by the apothem, then take half.")  // hint 2
T7S(TX_RP_SOL_AREA_PA, "A = P × a ÷ 2 = %d × %d ÷ 2 = %d cm².")  // the worked solution; %d perimeter, %d apothem, %d the answer
T7S(TX_RP_Q_AREA_NSA, "A regular polygon has %d sides of %d cm each and an apothem of %d cm (the apothem is the distance from the centre to a side). What is its area?")  // the question; %d sides, %d side length, %d apothem
T7S(TX_RP_SLIP_P_ONE, "That is the area of one of the triangles at the centre. The polygon has several of them.")  // a wrong answer: why
T7S(TX_RP_SLIP_P_PERIM, "That is the perimeter, not the area.")  // a wrong answer: why
T7S(TX_RP_SLIP_P_NUMS, "You added the three given numbers. The area comes from the perimeter and the apothem.")  // a wrong answer: why
T7S(TX_RP_HINT2_AREA_NSA, "First find the perimeter: the number of sides times the length of a side. Then multiply by the apothem and take half.")  // hint 2
T7S(TX_RP_SOL_AREA_NSA, "The perimeter is %d × %d = %d cm. Area: A = P × a ÷ 2 = %d × %d ÷ 2 = %d cm².")  // the worked solution; %d sides, %d side, %d perimeter, %d perimeter, %d apothem, %d the answer

// ---- The length of a circle (K_CIRCUMFERENCE)
T7S(TX_CF_Q_R, "A circle has radius %d cm. What is its length (circumference)? Give the answer in terms of π.")  // the question; %d radius
T7S(TX_CF_SLIP_NO2, "You left out the 2. The length of a circle is 2πr, not πr.")  // a wrong answer: why
T7S(TX_CF_SLIP_AREA, "You squared the radius. That belongs to the area, πr². The length is 2πr.")  // a wrong answer: why
T7S(TX_CF_SLIP_4R, "You doubled the radius twice. The length is 2 × π × r.")  // a wrong answer: why
T7S(TX_CF_SLIP_ADD, "You added instead of multiplying. In 2πr, the number 2 and the radius are multiplied.")  // a wrong answer: why
T7S(TX_CF_HINT1, "The length of a circle is 2πr, where r is the radius. What does the question give you?")  // hint 1
T7S(TX_CF_HINT2_R, "Multiply 2 by the radius. The answer keeps π as a factor.")  // hint 2
T7S(TX_CF_SOL_R, "L = 2πr = 2 × π × %d = %dπ cm.")  // the worked solution; %d radius, %d the answer
T7S(TX_CF_Q_D, "A circle has diameter %d cm. What is its length (circumference)? Give the answer in terms of π.")  // the question; %d diameter
T7S(TX_CF_SLIP_2D, "You doubled the diameter. In 2πr the 2 is already used up by the diameter: L = π × d.")  // a wrong answer: why
T7S(TX_CF_SLIP_RADIUS, "You used πr, with the radius in place of the diameter. The length is π times the diameter, which is 2πr.")  // a wrong answer: why
T7S(TX_CF_SLIP_DSQ, "You squared the diameter. That does not belong to the length of a circle.")  // a wrong answer: why
T7S(TX_CF_HINT2_D, "The diameter is twice the radius, and L = 2πr. You may also use L = π × d.")  // hint 2
T7S(TX_CF_SOL_D, "The radius is %d ÷ 2 = %d cm. L = 2πr = 2 × π × %d = %dπ cm.")  // the worked solution; %d diameter, %d radius, %d radius, %d the answer
T7S(TX_CF_Q_FROMLEN, "The length of a circle is %dπ cm. What is the radius of the circle?")  // the question; %d the number in front of π
T7S(TX_CF_SLIP_L_AS_R, "You took the number in front of π as the radius. L = 2πr, so that number is 2r.")  // a wrong answer: why
T7S(TX_CF_SLIP_L_TWICE, "You doubled the number in front of π. L = 2πr: to find r, divide by 2.")  // a wrong answer: why
T7S(TX_CF_SLIP_L_SQ, "You squared the radius. That belongs to the area, not to the length.")  // a wrong answer: why
T7S(TX_CF_SLIP_L_QUARTER, "You divided by 4. L = 2πr: divide the number in front of π by 2, once.")  // a wrong answer: why
T7S(TX_CF_HINT1_L, "The length of a circle is L = 2πr. Compare it with the given length: what is the number in front of π equal to?")  // hint 1
T7S(TX_CF_HINT2_L, "The number in front of π is 2r. Divide it by 2 to get the radius.")  // hint 2
T7S(TX_CF_SOL_L, "From L = 2πr: 2πr = %dπ, so 2r = %d and r = %d cm.")  // the worked solution; %d the number in front of π, %d the same, %d the answer
T7S(TX_CF_Q_WHEEL, "A wheel of radius %d cm makes %d full turns on the ground. What distance does it cover? Give the answer in terms of π.")  // the question; %d radius, %d turns
T7S(TX_CF_SLIP_W_NO2, "You left out the 2 of 2πr. In one turn the wheel covers the whole length of its circle.")  // a wrong answer: why
T7S(TX_CF_SLIP_W_ONE, "That is the distance for one turn only, but the wheel turns more than once.")  // a wrong answer: why
T7S(TX_CF_SLIP_W_AREA, "You squared the radius. The wheel rolls along its length, 2πr, not its area.")  // a wrong answer: why
T7S(TX_CF_SLIP_W_ADD, "You added the number of turns to the length of one turn. The turns multiply it.")  // a wrong answer: why
T7S(TX_CF_HINT1_W, "In one full turn, a wheel rolls a distance equal to the length of its circle.")  // hint 1
T7S(TX_CF_HINT2_W, "Find the length of the circle, 2πr, then multiply by the number of turns.")  // hint 2
T7S(TX_CF_SOL_W, "One turn: L = 2πr = 2 × π × %d = %dπ cm. In %d turns: %d × %dπ = %dπ cm.")  // the worked solution; %d radius, %d one turn's length, %d turns, %d turns, %d one turn's length, %d the answer
T7S(TX_CF_Q_TWO, "Two circles have radii %d cm and %d cm. How much longer is the larger circle than the smaller one? Give the answer in terms of π.")  // the question; %d the larger radius, %d the smaller radius
T7S(TX_CF_SLIP_T_NO2, "You subtracted the radii but left out the 2 of 2πr.")  // a wrong answer: why
T7S(TX_CF_SLIP_T_SUM, "You added the lengths of the two circles. The question asks how much longer one is, so subtract.")  // a wrong answer: why
T7S(TX_CF_SLIP_T_AREA, "You subtracted the squares of the radii. That belongs to the areas. The lengths use 2πr.")  // a wrong answer: why
T7S(TX_CF_SLIP_T_MIX, "You mixed the two lengths: twice one radius minus the other. Both circles have a length of the form 2πr.")  // a wrong answer: why
T7S(TX_CF_HINT1_T, "Find the length of each circle, then compare them.")  // hint 1
T7S(TX_CF_HINT2_T, "Subtract the two lengths. Both have the factor 2π, so you can subtract the radii and multiply by 2π.")  // hint 2
T7S(TX_CF_SOL_T, "The larger circle: 2 × π × %d = %dπ cm. The smaller: 2 × π × %d = %dπ cm. Difference: %dπ − %dπ = %dπ cm.")  // the worked solution; %d R, %d its length, %d r, %d its length, %d, %d the lengths, %d the answer

// ---- The area of a disc (K_DISC_AREA)
T7S(TX_DA_Q_R, "A circle has radius %d cm. What is the area of the disc? Give the answer in terms of π.")  // the question; %d radius
T7S(TX_DA_SLIP_LEN, "You used 2πr, the length of the circle. The area is πr².")  // a wrong answer: why
T7S(TX_DA_SLIP_NOSQ, "You forgot to square the radius. The area is πr², not πr.")  // a wrong answer: why
T7S(TX_DA_SLIP_DSQ, "You squared the diameter instead of the radius. The area is πr².")  // a wrong answer: why
T7S(TX_DA_SLIP_2R2, "You doubled the square of the radius. The area is π × r², with no extra factor.")  // a wrong answer: why
T7S(TX_DA_HINT1, "The area of a disc is πr², where r is the radius. What does the question give you?")  // hint 1
T7S(TX_DA_HINT2_R, "Square the radius. The answer keeps π as a factor.")  // hint 2
T7S(TX_DA_SOL_R, "A = πr² = π × %d² = %dπ cm².")  // the worked solution; %d radius, %d the answer
T7S(TX_DA_Q_D, "A circle has diameter %d cm. What is the area of the disc? Give the answer in terms of π.")  // the question; %d diameter
T7S(TX_DA_SLIP_D_ASR, "You used the diameter as the radius. The radius is half of the diameter.")  // a wrong answer: why
T7S(TX_DA_SLIP_D_NOSQ, "You multiplied π by the diameter instead of squaring the radius. The area is πr².")  // a wrong answer: why
T7S(TX_DA_SLIP_D_LEN, "You doubled the diameter. That has nothing to do with the area, πr².")  // a wrong answer: why
T7S(TX_DA_HINT2_D, "The radius is half of the diameter. Then use A = πr².")  // hint 2
T7S(TX_DA_SOL_D, "The radius is %d ÷ 2 = %d cm. A = πr² = π × %d² = %dπ cm².")  // the worked solution; %d diameter, %d radius, %d radius, %d the answer
T7S(TX_DA_Q_FROMAREA, "The area of a disc is %dπ cm². What is the radius of the circle?")  // the question; %d the number in front of π
T7S(TX_DA_SLIP_A_AS_R, "You took the number in front of π as the radius. In A = πr² that number is r².")  // a wrong answer: why
T7S(TX_DA_SLIP_A_TWICE, "You doubled the number in front of π. In A = πr² it is r², so take its square root.")  // a wrong answer: why
T7S(TX_DA_SLIP_A_HALF, "You halved the number in front of π, as for a length. For the area, r² is that number: take the square root.")  // a wrong answer: why
T7S(TX_DA_SLIP_A_QUARTER, "You divided by 4. The number in front of π is r²: take its square root.")  // a wrong answer: why
T7S(TX_DA_SLIP_A_DIAM, "That is the diameter, twice the radius.")  // a wrong answer: why
T7S(TX_DA_HINT1_A, "The area of a disc is πr². Compare it with the given area: what is the number in front of π equal to?")  // hint 1
T7S(TX_DA_HINT2_A, "The number in front of π is r². Which number multiplied by itself gives it?")  // hint 2
T7S(TX_DA_SOL_A, "A = πr² = %dπ, so r² = %d and r = %d cm, because %d² = %d.")  // the worked solution; %d the number in front of π, %d the same, %d the answer, %d the answer, %d the number
T7S(TX_DA_Q_FROMLEN, "The length of a circle is %dπ cm. What is the area of the disc? Give the answer in terms of π.")  // the question; %d the number in front of π
T7S(TX_DA_SLIP_L_SQ, "You squared the number in front of π. First find the radius: it is half of that number.")  // a wrong answer: why
T7S(TX_DA_SLIP_L_AS_AREA, "That is the number from the length. The area needs the radius squared.")  // a wrong answer: why
T7S(TX_DA_HINT1_L, "From the length L = 2πr you can find the radius. Then use the area formula.")  // hint 1
T7S(TX_DA_HINT2_L, "Find r from 2πr = L, then calculate πr².")  // hint 2
T7S(TX_DA_SOL_L, "From L = 2πr = %dπ: r = %d ÷ 2 = %d cm. A = πr² = π × %d² = %dπ cm².")  // the worked solution; %d the number in front of π, %d the same, %d radius, %d radius, %d the answer
T7S(TX_DA_Q_SQUARE, "A disc is inscribed in a square of side %d cm (the disc touches all four sides). What is the area of the disc? Give the answer in terms of π.")  // the question; %d side
T7S(TX_DA_SLIP_S_ASR, "You used the side of the square as the radius. The side equals the diameter, so the radius is half of it.")  // a wrong answer: why
T7S(TX_DA_SLIP_S_SIDE, "You multiplied π by the side of the square. The area of the disc is πr².")  // a wrong answer: why
T7S(TX_DA_SLIP_S_HALF, "You took half of the area of the square. The disc is round: its area is πr².")  // a wrong answer: why
T7S(TX_DA_HINT1_S, "The disc touches all four sides of the square. What is the diameter of the disc, compared with the side?")  // hint 1
T7S(TX_DA_HINT2_S, "The diameter equals the side of the square. Find the radius, then use A = πr².")  // hint 2
T7S(TX_DA_SOL_S, "The disc touches all four sides, so its diameter is the side of the square, %d cm. The radius is %d cm and A = πr² = π × %d² = %dπ cm².")  // the worked solution; %d side, %d radius, %d radius, %d the answer

// ---- Arcs and sectors (K_ARC_SECTOR)
T7S(TX_AS_Q_ARC, "A circle has radius %d cm. What is the length of an arc of %d°? Give the answer in terms of π.")  // the question; %d radius, %d the angle of the arc
T7S(TX_AS_SLIP_AREAFORM, "You used the area formula πr² instead of the length formula 2πr.")  // a wrong answer: why
T7S(TX_AS_SLIP_NO2, "You left out the 2 of 2πr. The arc is a fraction of the whole length 2πr.")  // a wrong answer: why
T7S(TX_AS_SLIP_DBL, "You took twice the right fraction of the circle. The arc is the angle over 360° of the whole length.")  // a wrong answer: why
T7S(TX_AS_SLIP_WHOLE, "That is the length of the whole circle. The arc is only a fraction of it.")  // a wrong answer: why
T7S(TX_AS_HINT1_ARC, "An arc is a fraction of the whole circle: its angle divided by the full turn.")  // hint 1
T7S(TX_AS_HINT2_ARC, "Find the length of the whole circle, 2πr, and take that fraction of it.")  // hint 2
T7S(TX_AS_SOL_ARC, "The arc is %d/360 of the circle: L = %d/360 × 2π × %d = %dπ cm.")  // the worked solution; %d the angle, %d the angle, %d radius, %d the answer
T7S(TX_AS_Q_SECTOR, "A circle has radius %d cm. What is the area of a sector of %d°? Give the answer in terms of π.")  // the question; %d radius, %d the angle of the sector
T7S(TX_AS_SLIP_S_DISC, "That is the area of the whole disc. The sector is only a fraction of it.")  // a wrong answer: why
T7S(TX_AS_SLIP_S_LENGTH, "You used the length formula 2πr instead of the area formula πr².")  // a wrong answer: why
T7S(TX_AS_SLIP_S_NOSQ, "You forgot to square the radius. The area of the disc is πr².")  // a wrong answer: why
T7S(TX_AS_SLIP_S_DBL, "You took twice the right fraction of the disc. The sector is the angle over 360° of the area.")  // a wrong answer: why
T7S(TX_AS_SLIP_S_HALF, "You took half of the right fraction of the disc. The sector is the angle over 360° of the area.")  // a wrong answer: why
T7S(TX_AS_HINT1_SECTOR, "A sector is a fraction of the whole disc: its angle divided by the full turn.")  // hint 1
T7S(TX_AS_HINT2_SECTOR, "Find the area of the whole disc, πr², and take that fraction of it.")  // hint 2
T7S(TX_AS_SOL_SECTOR, "The sector is %d/360 of the disc: A = %d/360 × π × %d² = %dπ cm².")  // the worked solution; %d the angle, %d the angle, %d radius, %d the answer
T7S(TX_AS_Q_FULL, "An arc of %d° of a circle has length %dπ cm. What is the length of the whole circle? Give the answer in terms of π.")  // the question; %d the angle, %d the number in front of π
T7S(TX_AS_SLIP_F_ARC, "That is the length of the arc itself. The whole circle is longer.")  // a wrong answer: why
T7S(TX_AS_SLIP_F_REST, "That is the length of the rest of the circle, without this arc. Add the arc back.")  // a wrong answer: why
T7S(TX_AS_SLIP_F_HALF, "You used 180° instead of 360° for the whole circle. That gives half of the circle, not all of it.")  // a wrong answer: why
T7S(TX_AS_SLIP_MULT, "You multiplied by the fraction of the circle. To go from the part to the whole, divide by that fraction.")  // a wrong answer: why
T7S(TX_AS_HINT1_FULL, "The arc (or sector) is a fraction of the whole circle: its angle divided by 360°. How many such pieces fill the circle?")  // hint 1
T7S(TX_AS_HINT2_FULL, "The whole circle is the arc length divided by its fraction of the circle, 360° ÷ the angle times the arc.")  // hint 2
T7S(TX_AS_SOL_FULL, "The arc is %d/360 of the whole circle, so L = arc × 360 ÷ %d = %dπ × 360 ÷ %d = %dπ cm.")  // the worked solution; %d the angle, %d the angle, %d the arc's number, %d the angle, %d the answer
T7S(TX_AS_Q_DISC, "A sector of %d° of a disc has area %dπ cm². What is the area of the whole disc? Give the answer in terms of π.")  // the question; %d the angle, %d the number in front of π
T7S(TX_AS_SLIP_D_SECTOR, "That is the area of the sector itself. The whole disc is bigger.")  // a wrong answer: why
T7S(TX_AS_SLIP_D_REST, "That is the area of the rest of the disc, without the sector. Add the sector back.")  // a wrong answer: why
T7S(TX_AS_SLIP_D_RADIUS, "That number is the radius of the disc. The area is π times the square of the radius.")  // a wrong answer: why
T7S(TX_AS_SLIP_D_HALF, "You used 180° instead of 360° for the whole disc. That gives half of the disc, not all of it.")  // a wrong answer: why
T7S(TX_AS_HINT2_DISC, "The whole disc is the sector's area divided by its fraction of the disc, 360° ÷ the angle times the sector.")  // hint 2
T7S(TX_AS_SOL_DISC, "The sector is %d/360 of the disc, so A = sector × 360 ÷ %d = %dπ × 360 ÷ %d = %dπ cm².")  // the worked solution; %d the angle, %d the angle, %d the sector's number, %d the angle, %d the answer
T7S(TX_AS_Q_RFROMARC, "An arc of %d° of a circle has length %dπ cm. What is the radius of the circle?")  // the question; %d the angle, %d the number in front of π
T7S(TX_AS_SLIP_R_AS_R, "You took the number in front of π as the radius. It belongs to the arc, which is only a fraction of the circle.")  // a wrong answer: why
T7S(TX_AS_SLIP_R_WHOLE, "That is the length of the whole circle, 2πr, not the radius.")  // a wrong answer: why
T7S(TX_AS_SLIP_R_SQ, "You squared the radius. That belongs to the area. The length of an arc uses the radius itself.")  // a wrong answer: why
T7S(TX_AS_HINT2_RARC, "First find the length of the whole circle, then use L = 2πr.")  // hint 2
T7S(TX_AS_SOL_RARC, "The arc is %d/360 of the circle, so the whole circle has length L = arc × 360 ÷ %d = %dπ × 360 ÷ %d = %dπ cm. From 2πr = %dπ: r = %d ÷ 2 = %d cm.")  // the worked solution; %d the angle, %d the angle, %d the arc's number, %d the angle, %d the length, %d the length, %d the length, %d the answer
T7S(TX_AS_Q_RFROMSECTOR, "A sector of %d° of a disc has area %dπ cm². What is the radius of the disc?")  // the question; %d the angle, %d the number in front of π
T7S(TX_AS_SLIP_RS_DISC, "That is the area of the whole disc, πr² with π left out. The radius is its square root.")  // a wrong answer: why
T7S(TX_AS_SLIP_RS_SECTOR, "You took the number in front of π as the radius. It belongs to the sector, which is only a fraction of the disc.")  // a wrong answer: why
T7S(TX_AS_SLIP_RS_DIAM, "That is the diameter, twice the radius.")  // a wrong answer: why
T7S(TX_AS_HINT2_RSEC, "First find the area of the whole disc, then use A = πr².")  // hint 2
T7S(TX_AS_SOL_RSEC, "The sector is %d/360 of the disc, so A = sector × 360 ÷ %d = %dπ × 360 ÷ %d = %dπ cm². From πr² = %dπ: r² = %d, so r = %d cm (%d² = %d).")  // the worked solution; %d the angle, %d the angle, %d the sector's number, %d the angle, %d the area, %d the area, %d the area, %d the answer, %d the answer, %d the area

// ---- The ring between two circles (K_ANNULUS)
T7S(TX_AN_Q_RADII, "Two concentric circles have radii %d cm and %d cm. What is the area of the ring between them? Give the answer in terms of π.")  // the question; %d the larger radius, %d the smaller radius
T7S(TX_AN_SLIP_DIFFSQ, "You squared the difference of the radii. The ring is the big disc minus the small disc: π(R² − r²).")  // a wrong answer: why
T7S(TX_AN_SLIP_SUM, "You added the areas of the two discs. The ring is what is left of the big disc when the small disc is cut out, so subtract.")  // a wrong answer: why
T7S(TX_AN_SLIP_ONESQ, "You squared only one radius. Both discs have an area with a squared radius: π(R² − r²).")  // a wrong answer: why
T7S(TX_AN_SLIP_NOSQ, "You subtracted the radii without squaring. The areas are πR² and πr².")  // a wrong answer: why
T7S(TX_AN_SLIP_BIGDISC, "That is the area of the big disc only. The small disc must be taken away.")  // a wrong answer: why
T7S(TX_AN_Q_DIAMS, "Two concentric circles have diameters %d cm and %d cm. What is the area of the ring between them? Give the answer in terms of π.")  // the question; %d the larger diameter, %d the smaller diameter
T7S(TX_AN_SLIP_D_AS_R, "You used the diameters as radii. The radius is half of the diameter.")  // a wrong answer: why
T7S(TX_AN_Q_WIDTH, "A ring is between two concentric circles. The outer circle has radius %d cm and the ring is %d cm wide. What is the area of the ring? Give the answer in terms of π.")  // the question; %d the outer radius, %d the width
T7S(TX_AN_SLIP_W_ONLY, "You used the width as a radius. The width is only the difference between the radii.")  // a wrong answer: why
T7S(TX_AN_SLIP_W_AS_r, "You took the width of the ring as the inner radius. The inner radius is the outer radius minus the width.")  // a wrong answer: why
T7S(TX_AN_SLIP_W_PROD, "You multiplied the outer radius by the width. That is not the area of a ring.")  // a wrong answer: why
T7S(TX_AN_HINT1, "The ring is what is left of the big disc when the small disc is cut out. What are the areas of the two discs?")  // hint 1
T7S(TX_AN_HINT2_W, "The inner radius is the outer radius minus the width of the ring. Then subtract the area of the small disc from the big one.")  // hint 2
T7S(TX_AN_HINT2, "Subtract the area of the small disc from the area of the big disc. Both have the factor π.")  // hint 2
T7S(TX_AN_HINT2_D, "The radii are half of the diameters. Subtract the area of the small disc from the big one.")  // hint 2
T7S(TX_AN_SOL_RADII, "The big disc has area π × %d² = %dπ. The small disc: π × %d² = %dπ. The ring: %dπ − %dπ = %dπ cm².")  // the worked solution; %d R, %d R², %d r, %d r², %d R², %d r², %d the answer
T7S(TX_AN_SOL_DIAMS, "The radii are %d ÷ 2 = %d cm and %d ÷ 2 = %d cm. Area of the ring: π × (%d² − %d²) = π × (%d − %d) = %dπ cm².")  // the worked solution; %d D, %d R, %d d, %d r, %d R, %d r, %d R², %d r², %d the answer
T7S(TX_AN_SOL_WIDTH, "The inner radius is %d − %d = %d cm. Area of the ring: π × (%d² − %d²) = π × (%d − %d) = %dπ cm².")  // the worked solution; %d R, %d width, %d r, %d R, %d r, %d R², %d r², %d the answer
// (Tutor, Unit 6: English rows, T7S(TX_ID, "text") one per line. Merged into t7_strings.h at the end.)
// Notation as in t7_strings.h: × ÷ for times and divided by, a decimal point, ° ² −. A fraction like 2/3 is a number. The reasons of
// wrong answers and the hints carry no numbers of the exercise.

// ---- Shared slips (Unit 6)
T7S(TX_U6_DEC, "%d.%d")  // a decimal number in a length, e.g. 0.3 dm; %d the whole part, %d the tenths digit
T7S(TX_U6_SLIP_ADD, "You added or subtracted the given numbers instead of using a proportion. A proportion makes ratios equal, not sums or differences.")  // a wrong answer: why
T7S(TX_U6_SLIP_COPY, "That is only one of the numbers in the problem. Here the quantities are proportional, not equal.")  // a wrong answer: why
T7S(TX_U6_SLIP_INV, "You turned one ratio upside down. In a proportion, the same kind of segment stays on top in both ratios.")  // a wrong answer: why
T7S(TX_U6_SLIP_MIX, "You compared a part with a whole side in one ratio and a part with a part in the other. Compare part with part, or part with whole, on both sides.")  // a wrong answer: why
T7S(TX_U6_SLIP_OTHER, "That is another quantity of the figure, not the one the question asks for. Re-read the question.")  // a wrong answer: why
T7S(TX_U6_SLIP_STOP, "You stopped after the first step. That value is only a stage of the solution: the question asks for another one.")  // a wrong answer: why

// ---- The ratio of two segments (K_RATIO)
T7S(TX_U6_RAT_Q0, "Segment AB measures %s and segment CD measures %s. What is the ratio %s?")  // the question; %s a length with its unit, %s a length with its unit, %s the ratio asked (AB/CD or CD/AB)
T7S(TX_U6_RAT_Q1, "Point M is on segment AB, with AM = %d cm and MB = %d cm. What is the ratio %s?")  // the question; %d AM, %d MB, %s the ratio asked (AM/MB, MB/AM, AM/AB or MB/AB)
T7S(TX_U6_RAT_Q3, "Point M is on segment AB, with AB = %d cm and AM = %d cm. What is the ratio %s?")  // the question; %d AB, %d AM, %s the ratio asked (AM/MB or MB/AM)
T7S(TX_U6_RAT_SLIP_INV, "You wrote the ratio upside down. The first segment named in the ratio goes on top.")  // a wrong answer: why
T7S(TX_U6_RAT_SLIP_SUM, "You divided by the sum of the two lengths. The ratio compares the two lengths themselves: the first divided by the second.")  // a wrong answer: why
T7S(TX_U6_RAT_SLIP_PARTS, "You compared the wrong segments. Read which two segments the ratio names: two parts, or a part and the whole segment.")  // a wrong answer: why
T7S(TX_U6_RAT_SLIP_DIFF, "You subtracted the given numbers. A ratio is a quotient (a division), not a difference.")  // a wrong answer: why
T7S(TX_U6_RAT_SLIP_LEN, "That is the length of a segment, not a ratio. The ratio of two lengths has no unit.")  // a wrong answer: why
T7S(TX_U6_RAT_SLIP_RAW, "You divided the numbers without converting. Both lengths must be in the same unit before you divide.")  // a wrong answer: why
T7S(TX_U6_RAT_SLIP_POW10, "The conversion is off by a factor of ten. Check how many of the smaller unit fit in the larger one.")  // a wrong answer: why
T7S(TX_U6_RAT_SLIP_SWAP, "You multiplied each length by the other length's conversion factor. Convert each length with the factor of its own unit.")  // a wrong answer: why
T7S(TX_U6_RAT_H1_0, "A ratio is the quotient of two lengths in the same unit. Which length goes on top?")  // hint 1
T7S(TX_U6_RAT_H2_0, "Write the fraction, then divide the numerator and the denominator by their common factors.")  // hint 2
T7S(TX_U6_RAT_H1_1, "The ratio compares the two segments named in it. Which two lengths are they, and which one goes on top?")  // hint 1
T7S(TX_U6_RAT_H2_1, "When the whole segment is involved, it is the sum of its parts: AB = AM + MB.")  // hint 2
T7S(TX_U6_RAT_H1_2, "A ratio needs both lengths in the same unit. Convert the larger unit to the smaller one: a decimeter is ten centimeters, a centimeter is ten millimeters, a meter is a hundred centimeters.")  // hint 1
T7S(TX_U6_RAT_H2_2, "Write the fraction with the two converted lengths, then simplify it.")  // hint 2
T7S(TX_U6_RAT_H1_3, "AB is the whole segment, the sum of the two parts: AB = AM + MB. Find MB first.")  // hint 1
T7S(TX_U6_RAT_H2_3, "Then divide the two lengths named in the ratio and simplify the fraction.")  // hint 2
T7S(TX_U6_RAT_SOL0, "%s = %s.")  // the worked solution; %s the ratio asked, %s the fraction worked out (e.g. 6/9 = 2/3)
T7S(TX_U6_RAT_SOL1, "AB = AM + MB = %d + %d = %d cm, so %s = %s.")  // the worked solution; %d AM, %d MB, %d AB, %s the ratio asked, %s the fraction worked out
T7S(TX_U6_RAT_SOL2, "In the same unit: AB = %s and CD = %s. Then %s = %s.")  // the worked solution; %s AB converted, %s CD converted, %s the ratio asked, %s the fraction worked out
T7S(TX_U6_RAT_SOL3, "MB = AB − AM = %d − %d = %d cm, so %s = %s.")  // the worked solution; %d AB, %d AM, %d MB, %s the ratio asked, %s the fraction worked out

// ---- Dividing a segment (K_DIVIDE_SEGMENT)
T7S(TX_U6_DIV_ASK_AM, "the length AM")  // what is asked, inside the question
T7S(TX_U6_DIV_ASK_MB, "the length MB")  // what is asked, inside the question
T7S(TX_U6_DIV_ASK_RATIO, "the ratio AM/AB")  // what is asked, inside the question
T7S(TX_U6_DIV_ASK3_AM, "the length AM")  // what is asked, inside the question
T7S(TX_U6_DIV_ASK3_MN, "the length MN")  // what is asked, inside the question
T7S(TX_U6_DIV_ASK3_NB, "the length NB")  // what is asked, inside the question
T7S(TX_U6_DIV_ASK3_MAX, "the length of the longest part")  // what is asked, inside the question
T7S(TX_U6_DIV_Q0, "Point M is on segment AB, with AM/MB = %d/%d and AB = %d cm. Find %s.")  // the question; %d a, %d b, %d AB, %s what is asked
T7S(TX_U6_DIV_Q1, "Segment AB = %d cm is divided by the points M and N into three parts AM, MN and NB, proportional to %d, %d and %d. Find %s.")  // the question; %d AB, %d a, %d b, %d c, %s what is asked
T7S(TX_U6_DIV_Q2, "Point M is on segment AB, with AB = %d cm and AM/AB = %d/%d. Find %s.")  // the question; %d AB, %d a, %d b, %s what is asked
T7S(TX_U6_DIV_SLIP_UNIT, "That is the length of one equal piece. A part has as many pieces as its number in the ratio, so multiply.")  // a wrong answer: why
T7S(TX_U6_DIV_SLIP_HALF, "You split the segment in half, as if M were the midpoint. The two parts are not equal here.")  // a wrong answer: why
T7S(TX_U6_DIV_SLIP_TERM, "You used a number of the ratio as a length. The ratio only says how many equal pieces each part has.")  // a wrong answer: why
T7S(TX_U6_DIV_SLIP_PARTWHOLE, "You used the ratio as if it compared a part with the whole segment AB. It compares the two parts, AM and MB.")  // a wrong answer: why
T7S(TX_U6_DIV_SLIP_PARTWHOLE2, "You used the ratio as if it compared the two parts. AM/AB compares AM with the whole segment AB.")  // a wrong answer: why
T7S(TX_U6_DIV_SLIP_PARTS, "That is the ratio of the two parts, but the question asks for the part compared with the whole segment AB.")  // a wrong answer: why
T7S(TX_U6_DIV_SLIP_OTHERRATIO, "That is the ratio of the other part, MB, to the whole segment. The question asks about AM.")  // a wrong answer: why
T7S(TX_U6_DIV_SLIP_THIRDS, "You cut the segment into three equal parts. The parts are proportional to different numbers, so they are not equal.")  // a wrong answer: why
T7S(TX_U6_DIV_H1_0, "The ratio says how many equal pieces each part has. AB is made of all the pieces together.")  // hint 1
T7S(TX_U6_DIV_H2_0, "Divide AB by the total number of pieces to get the length of one piece, then count the pieces you need.")  // hint 2
T7S(TX_U6_DIV_H1_1, "The three numbers say how many equal pieces each part has. AB is made of all the pieces together.")  // hint 1
T7S(TX_U6_DIV_H2_1, "Divide AB by the total number of pieces to get one piece, then multiply by the number of the part asked.")  // hint 2
T7S(TX_U6_DIV_H1_2, "AM/AB compares AM with the whole segment: AB is made of as many equal pieces as the second number, and AM has the first number of them.")  // hint 1
T7S(TX_U6_DIV_H2_2, "Divide AB by the second number to get one piece, then multiply by the number of pieces of the part asked (for MB, the pieces that are left).")  // hint 2
T7S(TX_U6_DIV_SOL_AM, "AM/MB = %d/%d, so AB is made of %d equal pieces. One piece is %d ÷ %d = %d cm. AM takes %d of them: AM = %d × %d = %d cm.")  // the worked solution; %d a, %d b, %d a + b, %d AB, %d a + b, %d one piece, %d a, %d a, %d one piece, %d AM
T7S(TX_U6_DIV_SOL_MB, "AM/MB = %d/%d, so AB is made of %d equal pieces. One piece is %d ÷ %d = %d cm. MB takes %d of them: MB = %d × %d = %d cm.")  // the worked solution; %d a, %d b, %d a + b, %d AB, %d a + b, %d one piece, %d b, %d b, %d one piece, %d MB
T7S(TX_U6_DIV_SOL_RATIO, "AM/MB = %d/%d, so AB is made of %d equal pieces. AM takes %d of the %d pieces, so AM/AB = %s.")  // the worked solution; %d a, %d b, %d a + b, %d a, %d a + b, %s the fraction
T7S(TX_U6_DIV_SOL3, "The parts are proportional to %d, %d and %d, so AB is made of %d + %d + %d = %d equal pieces. One piece is %d ÷ %d = %d cm. The part asked takes %d of the pieces: %d × %d = %d cm.")  // the worked solution; %d a, %d b, %d c, %d a, %d b, %d c, %d sum, %d AB, %d sum, %d one piece, %d the part's pieces, %d the part's pieces, %d one piece, %d answer
T7S(TX_U6_DIV_SOL2_AM, "From AM/AB, AB is made of %d equal pieces and AM has %d of them. One piece is %d ÷ %d = %d cm. AM = %d × %d = %d cm.")  // the worked solution; %d b, %d a, %d AB, %d b, %d one piece, %d a, %d one piece, %d AM
T7S(TX_U6_DIV_SOL2_MB, "From AM/AB, AB is made of %d equal pieces and AM has %d of them. One piece is %d ÷ %d = %d cm. MB takes %d − %d = %d of the pieces, so MB = %d × %d = %d cm.")  // the worked solution; %d b, %d a, %d AB, %d b, %d one piece, %d b, %d a, %d b − a, %d b − a, %d one piece, %d MB

// ---- Thales's theorem in the triangle (K_THALES)
T7S(TX_U6_THA_Q0, "In triangle ABC, D is on side AB and E is on side AC, with DE parallel to BC. AD = %d cm, DB = %d cm and %s = %d cm. How long is %s?")  // the question; %d AD, %d DB, %s the given segment (AE or EC), %d its length, %s the segment asked
T7S(TX_U6_THA_Q1, "In triangle ABC, D is on side AB and E is on side AC, with DE parallel to BC. AB = %d cm, AD = %d cm and %s = %d cm. How long is %s?")  // the question; %d AB, %d AD, %s the given segment (AE or AC), %d its length, %s the segment asked
T7S(TX_U6_THA_Q2, "In triangle ABC, D is on side AB and E is on side AC, with DE parallel to BC. AD = %d cm, DB = %d cm and AC = %d cm. How long is %s?")  // the question; %d AD, %d DB, %d AC, %s the segment asked (AE or EC)
T7S(TX_U6_THA_SLIP_WHOLE, "That is the length of the whole side AC, not of the part asked. Thales's theorem compares part with part: AD/DB = AE/EC.")  // a wrong answer: why
T7S(TX_U6_THA_H1_0, "DE is parallel to BC, so Thales's theorem gives proportional segments on the two sides. Which segments are parts of the same side?")  // hint 1
T7S(TX_U6_THA_H2_0, "Write AD/DB = AE/EC and cross-multiply to find the unknown segment.")  // hint 2
T7S(TX_U6_THA_H1_1, "You are given a whole side. First work out the missing part of that side: the whole minus the part you know.")  // hint 1
T7S(TX_U6_THA_H2_1, "Then use Thales's theorem: AD/DB = AE/EC (part to part), or AD/AB = AE/AC (part to whole side).")  // hint 2
T7S(TX_U6_THA_H1_2, "AB is not given, but AD and DB are. First find the whole side AB.")  // hint 1
T7S(TX_U6_THA_H2_2, "Then use the proportion AD/AB = AE/AC, or DB/AB = EC/AC: each part over its whole side.")  // hint 2
T7S(TX_U6_THA_SOL0A, "AD/DB = AE/EC, so EC = AE × DB ÷ AD = %d × %d ÷ %d = %d cm.")  // the worked solution; %d AE, %d DB, %d AD, %d EC
T7S(TX_U6_THA_SOL0B, "AD/DB = AE/EC, so AE = EC × AD ÷ DB = %d × %d ÷ %d = %d cm.")  // the worked solution; %d EC, %d AD, %d DB, %d AE
T7S(TX_U6_THA_SOL1A, "DB = AB − AD = %d − %d = %d cm. AD/DB = AE/EC, so EC = %d × %d ÷ %d = %d cm.")  // the worked solution; %d AB, %d AD, %d DB, %d AE, %d DB, %d AD, %d EC
T7S(TX_U6_THA_SOL1B, "AD/AB = AE/AC, so AE = AC × AD ÷ AB = %d × %d ÷ %d = %d cm.")  // the worked solution; %d AC, %d AD, %d AB, %d AE
T7S(TX_U6_THA_SOL1C, "DB = AB − AD = %d − %d = %d cm. DB/AB = EC/AC, so EC = %d × %d ÷ %d = %d cm.")  // the worked solution; %d AB, %d AD, %d DB, %d AC, %d DB, %d AB, %d EC
T7S(TX_U6_THA_SOL2A, "AB = AD + DB = %d + %d = %d cm. AD/AB = AE/AC, so AE = %d × %d ÷ %d = %d cm.")  // the worked solution; %d AD, %d DB, %d AB, %d AC, %d AD, %d AB, %d AE
T7S(TX_U6_THA_SOL2B, "AB = AD + DB = %d + %d = %d cm. DB/AB = EC/AC, so EC = %d × %d ÷ %d = %d cm.")  // the worked solution; %d AD, %d DB, %d AB, %d AC, %d DB, %d AB, %d EC

// ---- Thales's theorem in the X figure (K_THALES_X)
T7S(TX_U6_THX_Q, "Lines AB and CD cross at point O, and AC is parallel to BD. Given: %s. How long is %s?")  // the question; %s the given lengths (e.g. OA = 6 cm, OB = 9 cm and OC = 8 cm), %s the segment or line asked
T7S(TX_U6_THX_SLIP_WHOLE, "That is the length of a whole line, not of one part. In OA/OB = OC/OD, each ratio compares two parts.")  // a wrong answer: why
T7S(TX_U6_THX_SLIP_PARTONLY, "That is only one part of the line. The question asks for the whole line, the sum of its two parts.")  // a wrong answer: why
T7S(TX_U6_THX_SLIP_OTHERLINE, "That is the length of the other line. The question asks about the line that holds the unknown segment.")  // a wrong answer: why
T7S(TX_U6_THX_H1_0, "AC is parallel to BD, so the segments on the two lines are proportional: OA/OB = OC/OD. Which segment is unknown?")  // hint 1
T7S(TX_U6_THX_H1_1, "A whole line is given. First find the missing part of that line: the whole minus the part you know.")  // hint 1
T7S(TX_U6_THX_H1_5, "First find the missing part with OA/OB = OC/OD. The whole line is the sum of its two parts.")  // hint 1
T7S(TX_U6_THX_H2, "Cross-multiply the proportion OA/OB = OC/OD and divide to find the unknown segment.")  // hint 2
T7S(TX_U6_THX_SOL0, "OA/OB = OC/OD, so %s = %s × %s ÷ %s = %d × %d ÷ %d = %d cm.")  // the worked solution; %s the unknown, %s, %s, %s three given segments, %d, %d, %d their lengths, %d the answer
T7S(TX_U6_THX_SOL1, "%s = %s + %s, so %s = %d − %d = %d cm. Then OA/OB = OC/OD, so %s = %s × %s ÷ %s = %d × %d ÷ %d = %d cm.")  // the worked solution; %s the whole line, %s, %s two parts, %s the missing part, %d the whole, %d the known part, %d the missing part, %s the unknown, %s, %s, %s three segments, %d, %d, %d their lengths, %d the answer
T7S(TX_U6_THX_SOL5, "OA/OB = OC/OD, so %s = %s × %s ÷ %s = %d × %d ÷ %d = %d cm. The whole line is %s = %s + %s = %d + %d = %d cm.")  // the worked solution; %s the unknown part, %s, %s, %s three segments, %d, %d, %d their lengths, %d the part, %s the line asked, %s its two parts, %s, %d, %d, %d the sum

// ---- The converse of Thales's theorem (K_THALES_CONVERSE): the answers are statements
T7S(TX_U6_TCV_Q0, "In triangle ABC, D is on side AB and E is on side AC, with AD = %s, DB = %s, AE = %s and EC = %s. Are DE and BC parallel?")  // the question; %s four lengths with units
T7S(TX_U6_TCV_Q1, "In triangle ABC, D is on side AB and E is on side AC, with AB = %s, AD = %s, AC = %s and AE = %s. Are DE and BC parallel?")  // the question; %s four lengths with units
T7S(TX_U6_TCV_O1A, "Yes, because AD/DB = AE/EC")  // an answer
T7S(TX_U6_TCV_O2A, "No, because AD/DB differs from AE/EC")  // an answer
T7S(TX_U6_TCV_O1B, "Yes, because AD/AB = AE/AC")  // an answer
T7S(TX_U6_TCV_O2B, "No, because AD/AB differs from AE/AC")  // an answer
T7S(TX_U6_TCV_O3, "Yes, because AD/AB = AE/EC")  // an answer
T7S(TX_U6_TCV_O4, "Yes, because AD/DB = AE/AC")  // an answer
T7S(TX_U6_TCV_O5, "No, because AD is not equal to AE")  // an answer
T7S(TX_U6_TCV_O6, "No, because AD/AB differs from AE/EC")  // an answer
T7S(TX_U6_TCV_WHY_COMPARE, "Work out both ratios from the given lengths (in the same unit) and compare them. The converse of Thales's theorem decides by that comparison.")  // a wrong answer: why
T7S(TX_U6_TCV_WHY_MIX, "That compares a part with a whole side on one side of the triangle and a part with a part on the other. The converse of Thales's theorem compares the same kinds of segments on both sides.")  // a wrong answer: why
T7S(TX_U6_TCV_WHY_EQUAL, "The segments do not have to be equal. DE is parallel to BC when the ratios are equal, whatever the lengths: compare the ratios.")  // a wrong answer: why
T7S(TX_U6_TCV_H1_A, "The converse of Thales's theorem: if D and E are on the sides and AD/DB = AE/EC, then DE is parallel to BC. Work out both ratios.")  // hint 1
T7S(TX_U6_TCV_H1_B, "Compare AD/AB with AE/AC, each part with its whole side. First write all the lengths in the same unit.")  // hint 1
T7S(TX_U6_TCV_H2, "If the two ratios are equal, DE is parallel to BC. If they are different, it is not.")  // hint 2
T7S(TX_U6_TCV_SOL_YES_A, "AD/DB = %s and AE/EC = %s. The ratios are equal, so DE is parallel to BC (the converse of Thales's theorem).")  // the worked solution; %s the first ratio worked out, %s the second
T7S(TX_U6_TCV_SOL_NO_A, "AD/DB = %s and AE/EC = %s. The ratios are not equal, so DE is not parallel to BC (the converse of Thales's theorem).")  // the worked solution; %s the first ratio worked out, %s the second
T7S(TX_U6_TCV_SOL_YES_B, "In centimeters: AD/AB = %s and AE/AC = %s. The ratios are equal, so DE is parallel to BC (the converse of Thales's theorem).")  // the worked solution; %s the first ratio worked out, %s the second
T7S(TX_U6_TCV_SOL_NO_B, "In centimeters: AD/AB = %s and AE/AC = %s. The ratios are not equal, so DE is not parallel to BC (the converse of Thales's theorem).")  // the worked solution; %s the first ratio worked out, %s the second

// ---- The angle bisector theorem (K_BISECTOR)
T7S(TX_U6_BIS_Q0, "In triangle ABC, AD is the bisector of angle A, with D on side BC. AB = %d cm, AC = %d cm and BC = %d cm. How long is %s?")  // the question; %d AB, %d AC, %d BC, %s the segment asked (BD or DC)
T7S(TX_U6_BIS_Q1, "In triangle ABC, AD is the bisector of angle A, with D on side BC. BD = %d cm, DC = %d cm and %s = %d cm. How long is %s?")  // the question; %d BD, %d DC, %s the given side (AB or AC), %d its length, %s the side asked
T7S(TX_U6_BIS_Q2, "In triangle ABC, AD is the bisector of angle A, with D on side BC. AB = %d cm, AC = %d cm and %s = %d cm. How long is BC?")  // the question; %d AB, %d AC, %s the given segment (BD or DC), %d its length
T7S(TX_U6_BIS_SLIP_HALF, "You treated D as the midpoint of BC. The bisector divides BC in the ratio of the sides AB and AC, which are not equal.")  // a wrong answer: why
T7S(TX_U6_BIS_H1_0, "The angle bisector theorem: the bisector from A cuts BC into parts proportional to the sides next to it, BD/DC = AB/AC.")  // hint 1
T7S(TX_U6_BIS_H2_0, "Write BD/BC = AB/(AB + AC), the part of BC next to AB, and solve for BD (or DC/BC = AC/(AB + AC) for DC).")  // hint 2
T7S(TX_U6_BIS_H1_1, "Use the angle bisector theorem: BD/DC = AB/AC. Which three of the four lengths do you know?")  // hint 1
T7S(TX_U6_BIS_H2_1, "Cross-multiply and divide to find the unknown side.")  // hint 2
T7S(TX_U6_BIS_H1_2, "BC is the whole side: BC = BD + DC. First find the part you are not given.")  // hint 1
T7S(TX_U6_BIS_H2_2, "Use BD/DC = AB/AC to find the missing part, then add the two parts.")  // hint 2
T7S(TX_U6_BIS_SOL0, "BD/DC = AB/AC, so %s = BC × %s ÷ (AB + AC) = %d × %d ÷ (%d + %d) = %d cm.")  // the worked solution; %s the segment asked, %s the side next to it, %d BC, %d that side, %d AB, %d AC, %d the answer
T7S(TX_U6_BIS_SOL1, "BD/DC = AB/AC, so %s = %s × %s ÷ %s = %d × %d ÷ %d = %d cm.")  // the worked solution; %s the unknown side, %s, %s, %s three segments, %d, %d, %d their lengths, %d the answer
T7S(TX_U6_BIS_SOL2, "BD/DC = AB/AC, so %s = %s × %s ÷ %s = %d × %d ÷ %d = %d cm. Then BC = BD + DC = %d + %d = %d cm.")  // the worked solution; %s the missing part, %s, %s, %s three segments, %d, %d, %d their lengths, %d the missing part, %d BD, %d DC, %d BC

// ---- Similar triangles: a side (K_SIM_SIDE)
T7S(TX_U6_SS_Q, "Triangle ABC is similar to triangle %s, the vertices corresponding in the order written. In triangle ABC, %s = %d cm and %s = %d cm; in triangle %s, %s = %d cm. How long is %s?")  // the question; %s the second triangle's name (DEF, EFD...), %s a side of ABC, %d its length, %s another side of ABC, %d its length, %s the second triangle's name, %s the side of it that corresponds to the first, %d its length, %s the side asked
T7S(TX_U6_SS_SLIP_PAIR, "You used a side that does not correspond to the given one. The order of the letters tells which sides correspond.")  // a wrong answer: why
T7S(TX_U6_SS_SLIP_ORDER, "You matched the sides by the alphabetical order of the letters. The correspondence follows the order of the letters in the names of the triangles.")  // a wrong answer: why
T7S(TX_U6_SS_H1, "Write down which vertices correspond: the order of the letters in the names of the triangles tells you. Then find which sides correspond.")  // hint 1
T7S(TX_U6_SS_H2, "Corresponding sides are in the same ratio for every pair. Use the pair of corresponding sides you know to find the unknown side.")  // hint 2
T7S(TX_U6_SS_SOL, "The vertices correspond in the order written: A to %s, B to %s, C to %s. So %s corresponds to %s and %s corresponds to %s. Then %s/%s = %s/%s, so %s = %d × %d ÷ %d = %d cm.")  // the worked solution; %s, %s, %s the letters of the second triangle, %s side of ABC, %s its corresponding side, %s side of ABC, %s its corresponding side, then %s, %s, %s, %s the proportion (second side over first side, twice), %s the side asked, %d the given side of the second triangle, %d the side of ABC that goes with the asked one, %d the side of ABC that goes with the given one, %d the answer

// ---- The criteria of similarity (K_SIM_CRITERION): the answers are statements
T7S(TX_U6_CRI_L_UU, "Yes, by AA (two equal angles)")  // an answer
T7S(TX_U6_CRI_L_LUL, "Yes, by SAS (two proportional sides and the angle between them)")  // an answer
T7S(TX_U6_CRI_L_LLL, "Yes, by SSS (three proportional sides)")  // an answer
T7S(TX_U6_CRI_L_NO, "No, or not enough data")  // an answer
T7S(TX_U6_CRI_Q_UU, "In triangle ABC, angle %s = %d° and angle %s = %d°. In triangle DEF, angle %s = %d° and angle %s = %d°. Are the triangles similar?")  // the question; %s a vertex of ABC, %d its angle, %s another vertex, %d its angle, %s a vertex of DEF, %d its angle, %s another vertex, %d its angle
T7S(TX_U6_CRI_Q_LLL, "Triangle ABC has sides %d cm, %d cm and %d cm. Triangle DEF has sides %d cm, %d cm and %d cm. Are the triangles similar?")  // the question; three sides, three sides
T7S(TX_U6_CRI_Q_SAS, "Triangle ABC has AB = %d cm, AC = %d cm and angle A = %d°. Triangle DEF has DE = %d cm, DF = %d cm and angle D = %d°. Are the triangles similar?")  // the question; %d AB, %d AC, %d angle A, %d DE, %d DF, %d angle D
T7S(TX_U6_CRI_Q_TRAP, "Triangle ABC has AB = %d cm, BC = %d cm and angle A = %d°. Triangle DEF has DE = %d cm, EF = %d cm and angle D = %d°. Are the triangles similar?")  // the question; %d AB, %d BC, %d angle A, %d DE, %d EF, %d angle D
T7S(TX_U6_CRI_W_ANG_SAS, "SAS needs two proportional sides and the angle between them. Here only angles are given.")  // a wrong answer: why
T7S(TX_U6_CRI_W_ANG_SSS, "SSS needs the three sides of both triangles. Here only angles are given.")  // a wrong answer: why
T7S(TX_U6_CRI_W_ANG_NO, "Find the third angle of each triangle (the angles of a triangle add up to 180°) and compare. When two angles of one triangle equal two angles of the other, the triangles are similar.")  // a wrong answer: why
T7S(TX_U6_CRI_W_ANG_UU, "Find the third angle of each triangle and compare all the angles. AA needs two pairs of equal angles, not just one.")  // a wrong answer: why
T7S(TX_U6_CRI_W_SSS_UU, "No angles are given, so the angle criterion cannot be used.")  // a wrong answer: why
T7S(TX_U6_CRI_W_SSS_SAS, "SAS needs an angle, and none is given here.")  // a wrong answer: why
T7S(TX_U6_CRI_W_SSS_LLL, "Order the sides of each triangle by size, divide the corresponding sides and compare all three ratios. SSS needs all three to be equal.")  // a wrong answer: why
T7S(TX_U6_CRI_W_SSS_NO, "Order the sides of each triangle by size and compare the three ratios of corresponding sides. When all three are equal, the triangles are similar.")  // a wrong answer: why
T7S(TX_U6_CRI_W_SAS_UU, "Only one angle is given for each triangle. AA needs two pairs of equal angles.")  // a wrong answer: why
T7S(TX_U6_CRI_W_SAS_SSS, "Only two sides of each triangle are given. SSS needs all three sides.")  // a wrong answer: why
T7S(TX_U6_CRI_W_SAS_SAS, "SAS needs both: the two sides in proportion, and the equal angle between those two sides. Check each condition.")  // a wrong answer: why
T7S(TX_U6_CRI_W_SAS_NO, "Compare the ratios of the sides next to the angle. When they are equal and the angles between those sides are equal, SAS applies.")  // a wrong answer: why
T7S(TX_U6_CRI_H1_UU, "Two triangles are similar when two angles of one equal two angles of the other. Find the third angle of each triangle too.")  // hint 1
T7S(TX_U6_CRI_H2_UU, "The angles of a triangle add up to 180°. Compare the three angles of one triangle with the three angles of the other.")  // hint 2
T7S(TX_U6_CRI_H1_LLL, "To compare the sides, order the sides of each triangle from the smallest to the largest. Corresponding sides are in the same position.")  // hint 1
T7S(TX_U6_CRI_H2_LLL, "Divide each side of one triangle by the corresponding side of the other. The triangles are similar by SSS only if all three ratios are equal.")  // hint 2
T7S(TX_U6_CRI_H1_LUL, "SAS needs two sides and the angle between them. Find which sides are next to the given angle in each triangle.")  // hint 1
T7S(TX_U6_CRI_H2_LUL, "Compare the two ratios of corresponding sides, and check that the given angles are the ones between those sides.")  // hint 2
T7S(TX_U6_CRI_SOL_UU_YES, "The third angle is %d° in triangle ABC and %d° in triangle DEF. The angles of ABC are %d°, %d° and %d°, and those of DEF are %d°, %d° and %d°. The angles are equal in pairs, so the triangles are similar by AA.")  // the worked solution; %d third angle of ABC, %d third angle of DEF, three angles of ABC, three angles of DEF (each in increasing order)
T7S(TX_U6_CRI_SOL_UU_NO, "The third angle is %d° in triangle ABC and %d° in triangle DEF. The angles of ABC are %d°, %d° and %d°, and those of DEF are %d°, %d° and %d°. The angles are not equal in pairs, so the triangles are not similar.")  // the worked solution; %d third angle of ABC, %d third angle of DEF, three angles of ABC, three angles of DEF (each in increasing order)
T7S(TX_U6_CRI_SOL_LLL_YES, "Order the sides. Triangle ABC: %d, %d, %d cm. Triangle DEF: %d, %d, %d cm. The ratios of corresponding sides, %d/%d, %d/%d and %d/%d, are all equal to %s, so the triangles are similar by SSS.")  // the worked solution; three sides of ABC and three of DEF in increasing order, then the three ratios, then the common ratio
T7S(TX_U6_CRI_SOL_LLL_NO, "Order the sides. Triangle ABC: %d, %d, %d cm. Triangle DEF: %d, %d, %d cm. The ratios of corresponding sides, %d/%d, %d/%d and %d/%d, are not all equal, so the triangles are not similar by SSS.")  // the worked solution; three sides of ABC and three of DEF in increasing order, then the three ratios
T7S(TX_U6_CRI_SOL_SAS_YES, "AB/DE = %s and AC/DF = %s, so the ratios are equal. The angles between these sides are equal too (%d°), so the triangles are similar by SAS.")  // the worked solution; %s the first ratio worked out, %s the second, %d the angle
T7S(TX_U6_CRI_SOL_SAS_NOANG, "AB/DE = %s and AC/DF = %s, so the ratios are equal, but the angles between these sides are %d° and %d°, which are different. SAS does not apply.")  // the worked solution; %s the first ratio worked out, %s the second, %d angle A, %d angle D
T7S(TX_U6_CRI_SOL_SAS_NOSIDES, "AB/DE = %s and AC/DF = %s. The ratios are different, so SAS does not apply: these data do not show that the triangles are similar.")  // the worked solution; %s the first ratio worked out, %s the second
T7S(TX_U6_CRI_SOL_TRAP, "Angle A is between the sides AB and AC, and angle D is between DE and DF, but the given sides are AB, BC and DE, EF. The angle is not between the given sides, so SAS does not apply. The data are not enough to decide.")  // the worked solution

// ---- Perimeters and areas of similar triangles (K_SIM_PERIMETER_AREA)
T7S(TX_U6_SPA_Q0, "Triangle ABC is similar to triangle DEF, and AB/DE = %d/%d. The perimeter of triangle ABC is %d cm. What is the perimeter of triangle DEF?")  // the question; %d k's numerator, %d k's denominator, %d the perimeter of ABC
T7S(TX_U6_SPA_Q1, "Triangle ABC is similar to triangle DEF, and AB/DE = %d/%d. The perimeter of triangle DEF is %d cm. What is the perimeter of triangle ABC?")  // the question; %d k's numerator, %d k's denominator, %d the perimeter of DEF
T7S(TX_U6_SPA_Q2, "Triangles ABC and DEF are similar, with perimeters %d cm and %d cm. What is the ratio of similarity AB/DE?")  // the question; %d perimeter of ABC, %d perimeter of DEF
T7S(TX_U6_SPA_Q3, "Triangle ABC is similar to triangle DEF, and AB/DE = %d/%d. The area of triangle ABC is %d cm². What is the area of triangle DEF?")  // the question; %d k's numerator, %d k's denominator, %d the area of ABC
T7S(TX_U6_SPA_Q4, "Triangle ABC is similar to triangle DEF, and AB/DE = %d/%d. The area of triangle DEF is %d cm². What is the area of triangle ABC?")  // the question; %d k's numerator, %d k's denominator, %d the area of DEF
T7S(TX_U6_SPA_Q5, "Triangles ABC and DEF are similar, with areas %d cm² and %d cm². What is the ratio of similarity AB/DE?")  // the question; %d area of ABC, %d area of DEF
T7S(TX_U6_SPA_SLIP_SQUARE, "You squared the ratio for a perimeter. A perimeter changes by the ratio of similarity itself; only an area changes by its square.")  // a wrong answer: why
T7S(TX_U6_SPA_SLIP_WAY, "You used the ratio the wrong way round. AB/DE is the first triangle over the second: the triangle with the larger sides has the larger perimeter and the larger area.")  // a wrong answer: why
T7S(TX_U6_SPA_SLIP_NOSQUARE, "You used the ratio of similarity itself. Areas change by its square.")  // a wrong answer: why
T7S(TX_U6_SPA_SLIP_SQUARERATIO, "You squared the ratio. The ratio of the perimeters is the ratio of similarity itself, not its square.")  // a wrong answer: why
T7S(TX_U6_SPA_SLIP_AREARATIO, "That is the ratio of the areas, which is the square of the ratio of similarity. The ratio of similarity is its square root.")  // a wrong answer: why
T7S(TX_U6_SPA_H1_P, "In similar triangles all the sides are in the same ratio, so the perimeters are in that ratio too.")  // hint 1
T7S(TX_U6_SPA_H2_P, "Which triangle has the larger sides, ABC or DEF? Write the proportion between the two perimeters and solve it.")  // hint 2
T7S(TX_U6_SPA_H1_K, "The ratio of the perimeters of similar triangles equals the ratio of similarity.")  // hint 1
T7S(TX_U6_SPA_H2_K, "Write the perimeter of ABC over the perimeter of DEF, then simplify the fraction.")  // hint 2
T7S(TX_U6_SPA_H1_A, "For areas, the ratio is the square of the ratio of similarity.")  // hint 1
T7S(TX_U6_SPA_H2_A, "Square the ratio first, then use it as a proportion between the two areas.")  // hint 2
T7S(TX_U6_SPA_H1_AK, "The ratio of the areas is the square of the ratio of similarity.")  // hint 1
T7S(TX_U6_SPA_H2_AK, "Simplify the ratio of the areas to a fraction of two perfect squares, then take the square root of the numerator and of the denominator.")  // hint 2
T7S(TX_U6_SPA_SOL0, "The perimeters are in the ratio of similarity: P(ABC)/P(DEF) = AB/DE = %d/%d. So P(DEF) = %d × %d ÷ %d = %d cm.")  // the worked solution; %d k's numerator, %d k's denominator, %d P(ABC), %d denominator, %d numerator, %d the answer
T7S(TX_U6_SPA_SOL1, "The perimeters are in the ratio of similarity: P(ABC)/P(DEF) = AB/DE = %d/%d. So P(ABC) = %d × %d ÷ %d = %d cm.")  // the worked solution; %d k's numerator, %d k's denominator, %d P(DEF), %d numerator, %d denominator, %d the answer
T7S(TX_U6_SPA_SOL2, "The ratio of similarity is the ratio of the perimeters: AB/DE = P(ABC)/P(DEF) = %s.")  // the worked solution; %s the fraction worked out (e.g. 18/27 = 2/3)
T7S(TX_U6_SPA_SOL3, "The areas are in the ratio k squared: S(ABC)/S(DEF) = (%d/%d)² = %d/%d. So S(DEF) = %d × %d ÷ %d = %d cm².")  // the worked solution; %d numerator, %d denominator, %d numerator squared, %d denominator squared, %d S(ABC), %d denominator squared, %d numerator squared, %d the answer
T7S(TX_U6_SPA_SOL4, "The areas are in the ratio k squared: S(ABC)/S(DEF) = (%d/%d)² = %d/%d. So S(ABC) = %d × %d ÷ %d = %d cm².")  // the worked solution; %d numerator, %d denominator, %d numerator squared, %d denominator squared, %d S(DEF), %d numerator squared, %d denominator squared, %d the answer
T7S(TX_U6_SPA_SOL5, "S(ABC)/S(DEF) = %s. This is k squared, so k = AB/DE is the square root of it: k = %s.")  // the worked solution; %s the ratio of the areas worked out, %s the ratio of similarity

// ---- The fundamental theorem of similarity (K_TFA)
T7S(TX_U6_TFA_Q0, "In triangle ABC, D is on side AB and E is on side AC, with DE parallel to BC. AD = %d cm, AB = %d cm and %s = %d cm. How long is %s?")  // the question; %d AD, %d AB, %s the given segment (BC or DE), %d its length, %s the segment asked
T7S(TX_U6_TFA_Q1, "In triangle ABC, D is on side AB and E is on side AC, with DE parallel to BC. AD = %d cm, DB = %d cm and BC = %d cm. How long is DE?")  // the question; %d AD, %d DB, %d BC
T7S(TX_U6_TFA_Q2, "In triangle ABC, D is on side AB and E is on side AC, with DE parallel to BC. The perimeter of triangle ABC is %d cm, AD = %d cm and DB = %d cm. What is the perimeter of triangle ADE?")  // the question; %d perimeter of ABC, %d AD, %d DB
T7S(TX_U6_TFA_H1, "DE is parallel to BC, so triangle ADE is similar to triangle ABC. Which sides of the two triangles correspond?")  // hint 1
T7S(TX_U6_TFA_H2, "Corresponding sides are in the same ratio: AD/AB = DE/BC. Use the proportion that holds the unknown side.")  // hint 2
T7S(TX_U6_TFA_H1_2, "Triangle ADE is similar to triangle ABC, so their perimeters are in the ratio of the sides AD and AB. First find AB.")  // hint 1
T7S(TX_U6_TFA_H2_2, "The ratio of similarity is AD/AB. Apply it to the perimeter of the big triangle.")  // hint 2
T7S(TX_U6_TFA_SOL0A, "AD/AB = DE/BC (triangle ADE is similar to triangle ABC), so DE = BC × AD ÷ AB = %d × %d ÷ %d = %d cm.")  // the worked solution; %d BC, %d AD, %d AB, %d DE
T7S(TX_U6_TFA_SOL0B, "AD/AB = DE/BC (triangle ADE is similar to triangle ABC), so BC = DE × AB ÷ AD = %d × %d ÷ %d = %d cm.")  // the worked solution; %d DE, %d AB, %d AD, %d BC
T7S(TX_U6_TFA_SOL1, "AB = AD + DB = %d + %d = %d cm. AD/AB = DE/BC, so DE = %d × %d ÷ %d = %d cm.")  // the worked solution; %d AD, %d DB, %d AB, %d BC, %d AD, %d AB, %d DE
T7S(TX_U6_TFA_SOL2, "AB = AD + DB = %d + %d = %d cm. Triangle ADE is similar to triangle ABC with ratio AD/AB, so its perimeter is %d × %d ÷ %d = %d cm.")  // the worked solution; %d AD, %d DB, %d AB, %d the perimeter of ABC, %d AD, %d AB, %d the answer
