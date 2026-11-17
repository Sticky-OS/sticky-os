// Tutor's lessons in English: one LessonText a lesson, in slot order (0 is 7.2, the Pythagorean theorem;
// 1 to 8 are Unit 4's lessons 4.1 to 4.8). Included by tutor7_text.cpp. The plan of each lesson (its
// pages' figures, how many pages, its exercises) is in tutor7_lessons.cpp and is the same in every
// language: a language has exactly that many summary and example pages per lesson.
//
// A page: {"title", "body"}. In the body \n starts a paragraph; a square root is √ followed by its
// radicand (digits or letters) or a parenthesised expression (the screen draws the bar). A body
// stays under 620 bytes, so it fits a screen at 9 pt in portrait.
    // slot 0: 7.2
    {"The Pythagorean theorem",
     "Geometry",
     {
         {"The right triangle",
          "A right triangle has one right angle, an angle of 90°.\n"
          "The side opposite the right angle is called the hypotenuse. It is the longest side of the triangle. The other two sides are called the legs.\n"
          "In triangle ABC in the picture, angle A is the right angle: BC is the hypotenuse, and AB and AC are the legs."},
         {"The Pythagorean theorem",
          "In a right triangle, the square of the hypotenuse equals the sum of the squares of the legs.\n"
          "For triangle ABC, with A = 90°:\n"
          "BC² = AB² + AC²\n"
          "From this you can find any side: BC = √(AB² + AC²), AB = √(BC² − AC²), AC = √(BC² − AB²)."},
         {"How to use the theorem",
          "1. Find the right angle and say which side is the hypotenuse.\n"
          "2. Write the Pythagorean equation for that triangle.\n"
          "3. Put in the sides you know and calculate.\n"
          "4. Take the square root, and simplify it if you can: √32 = √(16 × 2) = 4√2.\n"
          "The hypotenuse is always longer than either leg."},
         {"The converse",
          "If, in a triangle, the squares of two sides add up to the square of the third side, then the triangle is a right triangle.\n"
          "The right angle is opposite the longest side. So when you check, use the longest side as the hypotenuse.\n"
          "Careful: all the sides must be in the same unit."},
         {"Special cases",
          "An isosceles right triangle with legs a has hypotenuse a√2. So the diagonal of a square with side s is s√2.\n"
          "An equilateral triangle with side s has height s√3/2: the height splits it into two right triangles, each with hypotenuse s and one leg s/2."},
     },
     {
         {"Example: find the hypotenuse",
          "In right triangle ABC, with A = 90°, AB = 9 cm and AC = 40 cm. What is BC?\n"
          "The hypotenuse is BC, so:\n"
          "BC² = AB² + AC² = 9² + 40² = 81 + 1600 = 1681\n"
          "BC = √1681 = 41 cm."},
         {"Example: find a leg",
          "In right triangle ABC, with A = 90°, BC = 9 cm and AB = 7 cm. What is AC?\n"
          "The hypotenuse is BC, so:\n"
          "AC² = BC² − AB² = 9² − 7² = 81 − 49 = 32\n"
          "AC = √32 = √(16 × 2) = 4√2 cm."},
         {"Example: is it a right triangle?",
          "A triangle has sides of 20 cm, 21 cm and 29 cm. Is it a right triangle?\n"
          "The longest side is 29 cm, so try it as the hypotenuse:\n"
          "20² + 21² = 400 + 441 = 841, and 29² = 841.\n"
          "The values are equal, so it is a right triangle, with the right angle opposite the 29 cm side."},
     }},
    // slot 1: 4.1
    {"The convex quadrilateral",
     "Geometry",
     {
         {"The quadrilateral",
          "A quadrilateral ABCD has four vertices (A, B, C, D) and four sides: AB, BC, CD, DA. Sides with no common vertex are opposite; sides that share a vertex are adjacent.\n"
          "The diagonals join opposite vertices: AC and BD.\n"
          "You can name it starting from any vertex, going either way (ABCD, BCDA, ADCB), but not ACBD."},
         {"Convex or concave",
          "A quadrilateral is convex if, for every side, the other two vertices are on the same side of the line through that side. Otherwise it is concave.\n"
          "In a convex quadrilateral, the segment joining any two points inside it stays inside.\n"
          "Every angle of a convex quadrilateral is less than 180°."},
         {"The sum of the angles",
          "Theorem: in a convex quadrilateral, the angles add up to 360°.\n"
          "Proof: diagonal BD splits the quadrilateral into two triangles, and the angles of each add up to 180°. So A + B + C + D = 180° + 180° = 360°."},
         {"How to use the theorem",
          "If you know three angles, the fourth is 360° minus their sum.\n"
          "If the angles are proportional to some numbers, divide 360° by the sum of the numbers to find what one part is worth. Each angle gets as many parts as its number says.\n"
          "Careful: don't use 180°; that is the sum for a triangle."},
     },
     {
         {"Example: the fourth angle",
          "A convex quadrilateral ABCD has angles A = 70°, B = 100° and C = 80°. What is D?\n"
          "A + B + C = 70° + 100° + 80° = 250°.\n"
          "D = 360° − 250° = 110°."},
         {"Example: angles in a ratio",
          "The angles of a convex quadrilateral are in the ratio 8 : 9 : 11 : 12. How big are they?\n"
          "Sum of the numbers: 8 + 9 + 11 + 12 = 40, so one part is 360° ÷ 40 = 9°.\n"
          "The angles: 72°, 81°, 99° and 108°."},
         {"Example: check the sum",
          "A quadrilateral has angles of 60°, 105°, 90° and 105°. Can it be convex?\n"
          "Their sum: 60° + 105° + 90° + 105° = 360°, and every angle is less than 180°.\n"
          "Yes, the sum is the right one for a convex quadrilateral."},
     }},
    // slot 2: 4.2
    {"The parallelogram",
     "Geometry",
     {
         {"Definition",
          "A parallelogram is a quadrilateral whose opposite sides are parallel in pairs: AB and CD are parallel, and BC and AD are parallel.\n"
          "Every parallelogram is a convex quadrilateral."},
         {"The angles",
          "Theorem: in a parallelogram a) consecutive angles are supplementary, so A + B = 180° (and the same for B + C, C + D, D + A); b) opposite angles are congruent: A = C and B = D.\n"
          "Converse: a convex quadrilateral whose consecutive angles are supplementary, or whose opposite angles are congruent in pairs, is a parallelogram."},
         {"The sides",
          "Theorem: in a parallelogram, opposite sides are congruent: AB = CD and AD = BC. The perimeter is P = 2 × (AB + BC).\n"
          "Converse: a convex quadrilateral whose opposite sides are congruent in pairs is a parallelogram.\n"
          "A convex quadrilateral with two opposite sides that are parallel and congruent is also a parallelogram."},
         {"The diagonals",
          "Theorem: in a parallelogram, the diagonals bisect each other. If they meet at O, then AO = OC and BO = OD.\n"
          "Converse: a convex quadrilateral whose diagonals bisect each other is a parallelogram."},
         {"How to recognize it",
          "A convex quadrilateral is a parallelogram if it has: opposite sides parallel; opposite sides congruent; consecutive angles supplementary; opposite angles congruent; diagonals that bisect each other; or two opposite sides that are parallel and congruent.\n"
          "Careful: just two opposite sides parallel, or just two opposite sides congruent, is not enough."},
     },
     {
         {"Example: the angles",
          "Parallelogram ABCD has angle A = 70°. What are B, C and D?\n"
          "B is consecutive to A: B = 180° − 70° = 110°.\n"
          "C is opposite A: C = 70°.\n"
          "D is opposite B: D = 110°."},
         {"Example: the diagonals",
          "The diagonals of parallelogram ABCD meet at O. You know AO = 3 cm and DO = 5 cm. What are CO and BD?\n"
          "The diagonals bisect each other: CO = AO = 3 cm.\n"
          "BO = DO = 5 cm, so BD = 2 × 5 = 10 cm."},
         {"Example: the perimeter",
          "A parallelogram has a perimeter of 30 cm, and one side is 6 cm. What are the other sides?\n"
          "Opposite sides are congruent, so two sides are 6 cm.\n"
          "The other two add up to 30 − 2 × 6 = 18 cm, so each is 9 cm."},
     }},
    // slot 3: 4.3
    {"The midsegment",
     "Geometry",
     {
         {"The midsegment",
          "A midsegment of a triangle joins the midpoints of two sides. Every triangle has three midsegments.\n"
          "Theorem: a midsegment is parallel to the third side and is half as long. If M is the midpoint of AB and N is the midpoint of AC, then MN = BC ÷ 2."},
         {"The medial triangle",
          "The three midsegments split the triangle into four congruent triangles.\n"
          "Each midsegment is half the side parallel to it, so the perimeter of the medial triangle (the triangle formed by the three midsegments) is half the perimeter of the original triangle."},
         {"The median",
          "A median of a triangle joins a vertex to the midpoint of the opposite side. Don't mix it up with a midsegment, which joins the midpoints of two sides.\n"
          "The three medians of a triangle meet at one point G, called the centroid."},
         {"The centroid",
          "Theorem: the centroid is on each median, two thirds of the way from the vertex and one third of the way from the opposite side. For median AM: GA = (2/3) × AM and GM = (1/3) × AM, so GA = 2 × GM.\n"
          "Careful: the 2 : 1 ratio is measured starting from the vertex."},
     },
     {
         {"Example: the midsegment",
          "In triangle ABC, M and N are the midpoints of sides AB and AC, and BC = 18 cm. What is MN?\n"
          "MN is a midsegment, so MN = BC ÷ 2 = 18 ÷ 2 = 9 cm.\n"
          "The other way round: if MN = 9 cm, then BC = 2 × 9 = 18 cm."},
         {"Example: the medial triangle",
          "Triangle ABC has sides of 10 cm, 14 cm and 12 cm. What is the perimeter of the triangle formed by the midsegments?\n"
          "The perimeter of ABC is 10 + 14 + 12 = 36 cm.\n"
          "The medial triangle's perimeter is half of that: 36 ÷ 2 = 18 cm."},
         {"Example: the centroid",
          "Median AM of triangle ABC is 18 cm, and G is the centroid. What are GA and GM?\n"
          "GA = (2/3) × 18 = 12 cm.\n"
          "GM = (1/3) × 18 = 6 cm. Check: 12 + 6 = 18 cm."},
     }},
    // slot 4: 4.4
    {"The rectangle",
     "Geometry",
     {
         {"Definition",
          "A rectangle is a parallelogram with a right angle. Every rectangle is a parallelogram, but not every parallelogram is a rectangle.\n"
          "Theorem: in a rectangle, all four angles are right angles (90°)."},
         {"The diagonals",
          "Theorem: the diagonals of a rectangle are congruent: AC = BD. They also bisect each other, so OA = OB = OC = OD, and triangles OAB, OBC, OCD, ODA are isosceles.\n"
          "Converse: a parallelogram with congruent diagonals is a rectangle."},
         {"How to recognize it",
          "It is a rectangle if it is: a parallelogram with a right angle; a parallelogram with congruent diagonals; a parallelogram with two congruent consecutive angles; or a convex quadrilateral with three right angles.\n"
          "Careful: just two right angles are not enough; the shape could be a right trapezoid."},
         {"Symmetry and angles",
          "A rectangle has two lines of symmetry: the perpendicular bisectors of its sides.\n"
          "The acute and obtuse angles between the diagonals are supplementary. In isosceles triangle AOB, the angle at O is the angle between the diagonals, and each base angle is (180° minus the angle at O) ÷ 2."},
     },
     {
         {"Example: the diagonals",
          "ABCD is a rectangle, and O is where the diagonals meet. If AO = 7 cm, what are AC and BD?\n"
          "The diagonals bisect each other: AC = 2 × AO = 14 cm.\n"
          "The diagonals are congruent: BD = AC = 14 cm."},
         {"Example: the angles",
          "The acute angle between the diagonals of a rectangle is 68°. What are the obtuse angle and the angle between a diagonal and the shorter side?\n"
          "The obtuse angle: 180° − 68° = 112°.\n"
          "Triangle AOB is isosceles: (180° − 68°) ÷ 2 = 56°."},
         {"Example: an equilateral triangle",
          "The diagonals of a rectangle are 18 cm and form a 60° angle. What is the perimeter of triangle AOB?\n"
          "OA = OB = 18 ÷ 2 = 9 cm, and the angle at O is 60°.\n"
          "Triangle AOB is equilateral: its perimeter is 3 × 9 = 27 cm."},
     }},
    // slot 5: 4.5
    {"The rhombus and the square",
     "Geometry",
     {
         {"The rhombus",
          "A rhombus is a parallelogram with two congruent adjacent sides.\n"
          "Theorem: all the sides of a rhombus are congruent. Converse: a quadrilateral with all sides congruent is a rhombus."},
         {"The rhombus diagonals",
          "Theorem: the diagonals of a rhombus are perpendicular, and each one bisects the angles it passes through. They are not congruent (except in a square).\n"
          "Converse: a parallelogram with perpendicular diagonals, or with a diagonal that bisects an angle, is a rhombus."},
         {"The square",
          "A square has all sides congruent and all angles right angles. It is both a rhombus and a rectangle, so it has all their properties: its diagonals are congruent, perpendicular and bisect the angles.\n"
          "Triangles OAB, OBC, OCD, ODA are isosceles right triangles. A square has four lines of symmetry."},
         {"How they fit together",
          "Every square is a rhombus and a rectangle; rhombuses and rectangles are parallelograms. So every square is a rhombus, but not every rhombus is a square.\n"
          "A rectangle with two congruent adjacent sides, or with perpendicular diagonals, is a square. A rhombus with a right angle, or with congruent diagonals, is a square."},
         {"Sides and diagonals",
          "In a rhombus, the diagonals cross at O, at right angles, at their midpoints. Triangle AOB is a right triangle with legs AO and BO (half of each diagonal), and side AB is the hypotenuse: AB² = AO² + BO².\n"
          "In a square, the diagonal is s√2 (by the Pythagorean theorem), so the side is d ÷ √2."},
     },
     {
         {"Example: rhombus angles",
          "ABCD is a rhombus, and diagonal AC makes a 35° angle with side AB. What are the angles of the rhombus?\n"
          "The diagonal bisects the angle: angle A = 2 × 35° = 70°.\n"
          "Consecutive angles are supplementary: angle B = 180° − 70° = 110°."},
         {"Example: rhombus side",
          "The diagonals of a rhombus are 12 cm and 16 cm. What are its side and perimeter?\n"
          "AO = 6 cm and BO = 8 cm. AB² = 6² + 8² = 36 + 64 = 100, so AB = 10 cm.\n"
          "The perimeter: 4 × 10 = 40 cm."},
         {"Example: square side",
          "The diagonal of a square is 10 cm. What is its side?\n"
          "By the Pythagorean theorem: 10² = s² + s² = 2 × s², so s² = 50.\n"
          "s = √50 = √(25 × 2) = 5√2 cm."},
     }},
    // slot 6: 4.6
    {"The trapezoid",
     "Geometry",
     {
         {"Definition",
          "A trapezoid is a convex quadrilateral with two opposite sides parallel, called the bases, while the other two sides are not parallel. The longer base is AB, the shorter base is CD, and AD and BC are the non-parallel sides.\n"
          "The bases have different lengths; otherwise the shape would be a parallelogram."},
         {"The angles",
          "In a trapezoid, the two angles along a non-parallel side are supplementary: A + D = 180° and B + C = 180°.\n"
          "Careful: angles A and B, on the same base, are usually not supplementary."},
         {"Kinds of trapezoids",
          "Right trapezoid: one non-parallel side is perpendicular to the bases (it has two right angles).\n"
          "Isosceles trapezoid: the non-parallel sides are congruent.\n"
          "The height of a trapezoid is a segment perpendicular to both bases. It has the same length wherever you draw it."},
         {"The midsegment",
          "The midsegment of a trapezoid joins the midpoints of the non-parallel sides.\n"
          "Theorem: it is parallel to the bases and MN = (AB + CD) ÷ 2.\n"
          "The part of it between the diagonals has length PQ = (AB − CD) ÷ 2."},
         {"Isosceles trapezoid",
          "In an isosceles trapezoid, the two angles on the same base are congruent (A = B and C = D), and the diagonals are congruent. It has one line of symmetry: the perpendicular bisector of the bases.\n"
          "The heights drawn from the ends of the shorter base cut off two pieces of the longer base, each (AB − CD) ÷ 2 long. The Pythagorean theorem then gives the height."},
     },
     {
         {"Example: the angles",
          "In trapezoid ABCD, with AB and CD parallel, angle A = 70° and angle C = 100°. What are B and D?\n"
          "A and D lie along side AD: D = 180° − 70° = 110°.\n"
          "B and C lie along side BC: B = 180° − 100° = 80°."},
         {"Example: the midsegment",
          "Trapezoid ABCD has bases AB = 12 cm and CD = 6 cm. What are the midsegment MN and the segment PQ between the diagonals?\n"
          "MN = (12 + 6) ÷ 2 = 9 cm.\n"
          "PQ = (12 − 6) ÷ 2 = 3 cm."},
         {"Example: the height",
          "Isosceles trapezoid ABCD has AB = 20 cm, CD = 8 cm and AD = BC = 10 cm. What is its height?\n"
          "The piece of the longer base under side AD (its projection): (20 − 8) ÷ 2 = 6 cm.\n"
          "h² = 10² − 6² = 100 − 36 = 64, so h = 8 cm."},
     }},
    // slot 7: 4.7
    {"Perimeter and area",
     "Geometry",
     {
         {"Perimeter",
          "The perimeter is the sum of the lengths of the sides. Triangle: P = a + b + c. Rectangle: P = 2 × (l + w). Square and rhombus: P = 4 × s. Parallelogram: P = 2 × (a + b). Trapezoid: the sum of its four sides.\n"
          "Before you calculate, put all the lengths in the same unit: 1 m = 10 dm = 100 cm."},
         {"Area",
          "Area measures the surface a shape covers. Rectangle: A = l × w. Square: A = s².\n"
          "Parallelogram: A = base × the height to that base. The height is perpendicular to the base; it is not the slanted side.\n"
          "Congruent shapes have the same area, but two shapes with the same area are not always congruent."},
         {"The triangle",
          "Area of a triangle: A = (side × the height to that side) ÷ 2. A diagonal splits a parallelogram into two congruent triangles, so each has half its area.\n"
          "Right triangle: A = (leg1 × leg2) ÷ 2. The height to the hypotenuse is h = (leg1 × leg2) ÷ hypotenuse."},
         {"Rhombus and trapezoid",
          "Rhombus: A = (d1 × d2) ÷ 2, half the product of the diagonals. Since it is a parallelogram, also A = side × height.\n"
          "Trapezoid: A = ((B + b) × h) ÷ 2, which is the midsegment times the height."},
         {"Common mistakes",
          "Don't forget to divide by two for a triangle and a rhombus.\n"
          "The height is not the slanted side.\n"
          "Perimeter and area are independent: two shapes can have the same area and different perimeters.\n"
          "Area is written in cm², perimeter in cm."},
     },
     {
         {"Example: parallelogram",
          "Parallelogram ABCD has base AB = 10 cm, side BC = 7 cm, and the height to AB is 6 cm. What are its area and perimeter?\n"
          "A = 10 × 6 = 60 cm².\n"
          "P = 2 × (10 + 7) = 34 cm."},
         {"Example: rhombus",
          "A rhombus has diagonals of 12 cm and 16 cm. What are its side, perimeter and area?\n"
          "Side: 6² + 8² = 100, so 10 cm. Perimeter: 4 × 10 = 40 cm.\n"
          "Area: (12 × 16) ÷ 2 = 96 cm²."},
         {"Example: trapezoid and units",
          "A trapezoid has bases of 10 cm and 0.7 dm, and a height of 4 cm. What is its area?\n"
          "0.7 dm = 7 cm.\n"
          "A = ((10 + 7) × 4) ÷ 2 = 68 ÷ 2 = 34 cm²."},
     }},
    // slot 8: 4.8
    {"Unit 4 review",
     "Geometry",
     {
         {"What proves what",
          "Parallelogram: opposite sides parallel or congruent, opposite angles congruent, consecutive angles supplementary, diagonals that bisect each other, or two opposite sides parallel and congruent.\n"
          "Rectangle: a parallelogram with a right angle or with congruent diagonals.\n"
          "Rhombus: a parallelogram with two congruent adjacent sides or with perpendicular diagonals.\n"
          "Square: a rectangle and a rhombus at the same time."},
         {"Formulas",
          "Angles of a convex quadrilateral: 360°. Midsegment of a triangle: BC ÷ 2. Centroid: GA = (2/3) × AM.\n"
          "Midsegment of a trapezoid: (B + b) ÷ 2.\n"
          "Areas: parallelogram base × h; triangle (base × h) ÷ 2; rhombus (d1 × d2) ÷ 2; trapezoid ((B + b) × h) ÷ 2."},
         {"Common mistakes",
          "Consecutive angles: supplementary; opposite angles: congruent.\n"
          "A midsegment is not a median.\n"
          "The 2 : 1 ratio on a median is measured from the vertex.\n"
          "Two right angles alone don't make a rectangle.\n"
          "The diagonals of a rhombus are not congruent.\n"
          "For areas: don't forget to divide by two."},
     },
     {
         {"Example: which shape is it?",
          "Parallelogram ABCD has congruent, perpendicular diagonals. Which shape is it?\n"
          "Congruent diagonals: a rectangle. Perpendicular diagonals: a rhombus.\n"
          "Since it is both a rectangle and a rhombus, ABCD is a square."},
         {"Example: two angles",
          "In parallelogram ABCD, angle A is twice angle B. What are they?\n"
          "A and B are consecutive: A + B = 180°, so 2 × B + B = 180°.\n"
          "3 × B = 180°, so B = 60° and A = 120°."},
         {"Example: height to hypotenuse",
          "A right triangle has legs of 6 cm and 8 cm and a hypotenuse of 10 cm. What is the height to the hypotenuse?\n"
          "Area: (6 × 8) ÷ 2 = 24 cm².\n"
          "The area is also (10 × h) ÷ 2, so h = 48 ÷ 10 = 4.8 cm."},
     }},
