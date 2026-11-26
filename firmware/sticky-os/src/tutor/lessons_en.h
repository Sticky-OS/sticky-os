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

// Unit 5 (the circle) lessons in English, one LessonText per slot (9..14: 5.1 to 5.6). Included after Unit 4's.
    // slot 9: 5.1
    {"Chords and arcs",
     "Geometry",
     {
         {"The parts of a circle",
          "A circle C(O, r) is the set of all points of the plane at distance r from the point O. O is the centre and r is the radius; all radii are equal.\n"
          "A chord is a segment with both ends on the circle (AB in the picture). A chord through the centre is a diameter: it is the longest chord, and its length is 2r.\n"
          "A central angle has its vertex at O, like angle AOB."},
         {"Central angle and arcs",
          "Two points A and B split the circle into two arcs: the minor arc and the major arc.\n"
          "The measure of the minor arc AB equals the central angle AOB. The major arc is 360° minus the minor arc. The arcs that go all round the circle add up to 360°.\n"
          "The ends of a diameter give two semicircles of 180° each.\n"
          "Careful: if angle AOB = 70°, the major arc is 290°, not 70°."},
         {"Equal arcs, equal chords",
          "In the same circle (or in circles with equal radii), congruent arcs have congruent chords, and congruent chords have congruent arcs.\n"
          "A line through the centre that is perpendicular to a chord bisects it: in the picture M is the midpoint of AB, so AM = MB = AB ÷ 2.\n"
          "Careful: arcs of equal measure are congruent only if the circles have equal radii."},
         {"Distance to the chord",
          "The distance from the centre to a chord is the length of the perpendicular OM from O to the chord AB. Its foot M is the midpoint of AB.\n"
          "Congruent chords of a circle are equally far from the centre, and chords equally far from the centre are congruent.\n"
          "The longer the chord, the closer it is to the centre; for a diameter the distance is 0."},
         {"The chord by Pythagoras",
          "Triangle OMA has a right angle at M, with OA = r, OM = d and AM = AB ÷ 2. So:\n"
          "d² + (AB ÷ 2)² = r²\n"
          "To find the chord: AM = √(r² − d²), then AB = 2 × AM.\n"
          "To find the distance: d = √(r² − (AB ÷ 2)²).\n"
          "Careful: the chord is twice AM, not AM. And d is always less than r."},
     },
     {
         {"Example: the two arcs",
          "O is the centre of a circle, and A, B are points on it with angle AOB = 70°. Find the two arcs AB.\n"
          "The minor arc equals the central angle: 70°.\n"
          "The major arc is 360° − 70° = 290°.\n"
          "Check: 70° + 290° = 360°."},
         {"Example: the chord",
          "A circle has radius r = 13 cm. A chord AB is at distance OM = 5 cm from the centre O. How long is AB?\n"
          "M is the midpoint of AB, and in the right triangle OMA:\n"
          "AM² = 13² − 5² = 169 − 25 = 144, so AM = 12 cm.\n"
          "AB = 2 × 12 = 24 cm."},
         {"Example: the distance",
          "A circle has radius 10 cm and a chord AB = 16 cm. How far is AB from the centre?\n"
          "AM = 16 ÷ 2 = 8 cm.\n"
          "OM² = 10² − 8² = 100 − 64 = 36\n"
          "OM = 6 cm."},
     }},
    // slot 10: 5.2
    {"The inscribed angle",
     "Geometry",
     {
         {"The inscribed angle",
          "An inscribed angle has its vertex on the circle and its sides contain chords. In the picture, angle BAC is inscribed; it intercepts the arc BC, the arc between its sides.\n"
          "Theorem: an inscribed angle is half the arc it intercepts.\n"
          "angle BAC = arc BC ÷ 2, so arc BC = 2 × angle BAC.\n"
          "It is also half of the central angle BOC on the same arc."},
         {"Semicircle and equal angles",
          "If BC is a diameter, the arc BC is 180°, so angle BAC = 180° ÷ 2 = 90°. An angle inscribed in a semicircle is a right angle.\n"
          "Inscribed angles that intercept the same arc are congruent, because each one is half of that arc. In the picture, any other point A on the same side of BC gives the same angle BAC."},
         {"The inscribed quadrilateral",
          "A quadrilateral ABCD with all four vertices on a circle is inscribed in the circle (the circle is circumscribed about it).\n"
          "Theorem: opposite angles of an inscribed quadrilateral add up to 180°.\n"
          "A + C = 180° and B + D = 180°\n"
          "Why: angle A is half of the arc BCD, angle C is half of the arc BAD, and those two arcs make 360°.\n"
          "Not every quadrilateral can be inscribed in a circle."},
         {"Pitfalls",
          "1. Central or inscribed? The central angle equals its arc; the inscribed angle is half of it.\n"
          "2. Minor or major arc? The inscribed angle takes the arc between its sides. If the angle is obtuse, that arc is more than 180°.\n"
          "3. The vertex on the other side of the chord BC gives the supplementary angle: 180° − angle BAC.\n"
          "4. Halve once only: arc ÷ 2, or angle × 2."},
     },
     {
         {"Example: the arc",
          "An angle BAC of 65° is inscribed in a circle with centre O. Find the arc BC and the central angle BOC.\n"
          "The arc is twice the inscribed angle: arc BC = 2 × 65° = 130°.\n"
          "The central angle equals its arc, so angle BOC = 130°."},
         {"Example: the inscribed quadrilateral",
          "ABCD is inscribed in a circle, with A = 78° and B = 105°. Find C and D.\n"
          "Opposite angles are supplementary:\n"
          "C = 180° − 78° = 102°\n"
          "D = 180° − 105° = 75°\n"
          "Check: 78° + 105° + 102° + 75° = 360°."},
         {"Example: the diameter",
          "Triangle ABC is inscribed in a circle, and BC is a diameter. Angle B = 35°. Find A and C.\n"
          "The angle in a semicircle is right: A = 90°.\n"
          "The angles of a triangle add up to 180°:\n"
          "C = 180° − 90° − 35° = 55°."},
     }},
    // slot 11: 5.3
    {"Tangents",
     "Geometry",
     {
         {"Line and circle",
          "Let d be the distance from the centre O to a line, and r the radius.\n"
          "If d < r, the line is a secant: it meets the circle in two points.\n"
          "If d = r, the line is a tangent: it has exactly one common point with the circle, the point of tangency.\n"
          "If d > r, the line is exterior: no common point.\n"
          "To decide, compare the distance with the radius."},
         {"The tangent and the radius",
          "Theorem: a tangent is perpendicular to the radius drawn to the point of tangency.\n"
          "In the picture, AC is tangent at C, so triangle OCA has a right angle at C. By Pythagoras:\n"
          "OA² = r² + AC²\n"
          "tangent length: AC = √(OA² − r²)\n"
          "From a point outside the circle you can draw two tangents; from a point on the circle, one; from a point inside, none."},
         {"Two tangents from a point",
          "Theorem: the two tangents AC and AD from an outside point A are equal: AC = AD (the right triangles OCA and ODA are congruent).\n"
          "The line OA bisects angle CAD and angle COD, and it is the perpendicular bisector of CD.\n"
          "So angle CAD + angle COD = 180°.\n"
          "Careful: the equal segments are the two tangents, not a tangent and a radius."},
         {"The circumscribed quadrilateral",
          "A quadrilateral ABCD whose four sides are tangent to a circle is circumscribed about it.\n"
          "Theorem: AB + CD = BC + AD. The sums of the opposite sides are equal.\n"
          "Why: each vertex has two equal tangent segments, say x, y, z, t. Then AB + CD = x + y + z + t = BC + AD.\n"
          "Use: AD = AB + CD − BC.\n"
          "Careful: add opposite sides, never adjacent ones."},
         {"The area of a triangle",
          "A circle inscribed in a triangle touches all three sides. Its centre is where the angle bisectors meet, and its radius r is perpendicular to each side.\n"
          "A = p × r\n"
          "where p is the semiperimeter, p = (a + b + c) ÷ 2.\n"
          "Why: three triangles with the centre as vertex have bases a, b, c and height r.\n"
          "Careful: p is half the perimeter, not the perimeter. The unit is cm²."},
     },
     {
         {"Example: the tangent length",
          "A circle has centre O and radius 8 cm. From a point A with OA = 17 cm the tangents AC and AD are drawn. Find AC.\n"
          "The radius OC is perpendicular to the tangent AC.\n"
          "AC² = OA² − OC² = 17² − 8² = 289 − 64 = 225\n"
          "AC = 15 cm, and AD = 15 cm too."},
         {"Example: the circumscribed quadrilateral",
          "ABCD is circumscribed about a circle, with AB = 9 cm, BC = 6 cm and CD = 7 cm. Find AD.\n"
          "AB + CD = BC + AD, so 9 + 7 = 6 + AD.\n"
          "AD = 9 + 7 − 6 = 10 cm."},
         {"Example: the area",
          "A triangle has sides 10 cm, 12 cm and 14 cm, and the inscribed circle has radius 4 cm. Find the area.\n"
          "p = (10 + 12 + 14) ÷ 2 = 18 cm.\n"
          "A = p × r = 18 × 4 = 72 cm²."},
     }},
    // slot 12: 5.4
    {"Regular polygons in a circle",
     "Geometry",
     {
         {"Regular polygons",
          "A regular polygon has all its sides congruent and all its angles congruent. Examples: the equilateral triangle and the square.\n"
          "A regular polygon with n sides is inscribed in a circle when its vertices A1, A2, ..., An are on the circle; they divide it into n congruent arcs. The centre O of the circle is the centre of the polygon.\n"
          "Careful: a rhombus has equal sides but is not regular; a rectangle has equal angles but is not regular."},
         {"The central angle",
          "Each side of the polygon cuts off an arc of 360° ÷ n, so each central angle, such as angle A1OA2, is\n"
          "360° ÷ n\n"
          "For n = 6 it is 60°; for n = 10 it is 36°.\n"
          "The triangles OA1A2, OA2A3, ... are congruent and isosceles, with two sides equal to the radius.\n"
          "Check: the n central angles add up to 360°."},
         {"The angle of the polygon",
          "The angle of a regular polygon with n sides is\n"
          "(n − 2) × 180° ÷ n\n"
          "and the sum of all its angles is (n − 2) × 180°.\n"
          "For the pentagon: 3 × 180° ÷ 5 = 108°.\n"
          "If the angle is given, write the equation and solve it for n; n must be a whole number.\n"
          "Careful: don't mix it up with the central angle 360° ÷ n."},
         {"The hexagon and the apothem",
          "In a regular hexagon inscribed in a circle of radius r, the central angle is 360° ÷ 6 = 60°. Triangle OAB is isosceles with a 60° angle, so it is equilateral: the side of the hexagon equals the radius, AB = r. Each angle of the hexagon is 120°.\n"
          "The apothem is the segment from the centre O to the midpoint of a side: the distance from O to that side.\n"
          "Area of a regular polygon = perimeter × apothem ÷ 2."},
     },
     {
         {"Example: the pentagon",
          "A regular pentagon is inscribed in a circle. Find its central angle and its angle.\n"
          "Central angle: 360° ÷ 5 = 72°.\n"
          "Angle of the pentagon: (5 − 2) × 180° ÷ 5 = 540° ÷ 5 = 108°."},
         {"Example: how many sides?",
          "Each angle of a regular polygon is 140°. How many sides does it have?\n"
          "(n − 2) × 180° ÷ n = 140°\n"
          "180n − 360 = 140n, so 40n = 360 and n = 9.\n"
          "Check: 7 × 180° ÷ 9 = 140°. It is a regular nonagon."},
         {"Example: the hexagon",
          "A regular hexagon is inscribed in a circle of radius 6 cm. Find its side and perimeter.\n"
          "The central angle is 360° ÷ 6 = 60°, so triangle OAB is equilateral.\n"
          "AB = OA = 6 cm.\n"
          "Perimeter = 6 × 6 = 36 cm."},
     }},
    // slot 13: 5.5
    {"Circumference and area of a disc",
     "Geometry",
     {
         {"The length of the circle",
          "The ratio of the length of a circle to its diameter is the same for every circle. It is the number π (pi), about 3.14; it is not an exact fraction.\n"
          "L = 2πr\n"
          "Example: r = 5 cm gives L = 10π cm, which is about 31.4 cm.\n"
          "Careful: use the radius r (the diameter is 2r). Keep π in the answer unless you are asked to approximate, and round only at the end."},
         {"The area of the disc",
          "A = πr²\n"
          "Example: r = 5 cm gives A = 25π cm², which is about 78.5 cm².\n"
          "Careful: a length is in cm, an area in cm². Square r first, then multiply by π: πr² is not (πr)².\n"
          "To find r: from L, r = L ÷ (2π); from A, r² = A ÷ π."},
         {"The length of an arc",
          "An arc of n° is n/360 of the whole circle, so its length is\n"
          "(n/360) × 2πr\n"
          "The semicircle (n = 180) has length πr.\n"
          "Example: n = 60°, r = 6 cm: (60/360) × 2π × 6 = 2π cm.\n"
          "A sector is the part of the disc between two radii and the arc between them (the sector OAB in the picture)."},
         {"The area of a sector",
          "The sector of n° is n/360 of the disc, so its area is\n"
          "(n/360) × πr²\n"
          "Example: n = 60°, r = 6 cm: (60/360) × π × 36 = 6π cm².\n"
          "Careful: the arc length has 2πr, the sector area has πr². Don't mix them up: the length is in cm, the area in cm²."},
         {"The ring",
          "Two circles with the same centre, radii R (big) and r (small), bound a ring. Its area is the big disc minus the small one:\n"
          "πR² − πr² = π(R² − r²)\n"
          "Example: R = 5 cm, r = 3 cm: π(25 − 9) = 16π cm².\n"
          "Careful: square first, then subtract; π(R − r)² is wrong.\n"
          "If you replace π by 3.14, do it only at the end."},
     },
     {
         {"Example: length and area",
          "A circle has radius 9 cm. Find its length and the area of the disc.\n"
          "L = 2 × π × 9 = 18π cm\n"
          "A = π × 9² = 81π cm²\n"
          "With π taken as 3.14, L is about 56.5 cm."},
         {"Example: arc and sector",
          "A circle has radius 6 cm. Find the length of an arc of 120° and the area of the sector of 120°.\n"
          "120° is 120 ÷ 360 = 1/3 of the circle.\n"
          "Arc: (1/3) × 2π × 6 = (1/3) × 12π = 4π cm.\n"
          "Sector: (1/3) × π × 36 = 12π cm²."},
         {"Example: the ring",
          "Two concentric circles have radii R = 10 cm and r = 6 cm. Find the area of the ring.\n"
          "A = π(R² − r²) = π(100 − 36)\n"
          "100 − 36 = 64\n"
          "A = 64π cm²."},
     }},
    // slot 14: 5.6
    {"Review of the unit",
     "Geometry",
     {
         {"Angles, arcs, chords",
          "Central angle = its arc. Inscribed angle = its arc ÷ 2. The angle in a semicircle is 90°. Opposite angles of an inscribed quadrilateral add up to 180°.\n"
          "Chord: d² + (chord ÷ 2)² = r², with d the distance from the centre. Equal chords are equally far from the centre.\n"
          "Use: with an arc or an angle, double or halve; with a chord and its distance, use Pythagoras with half the chord."},
         {"Tangents and polygons",
          "Tangent: perpendicular to the radius, so tangent² = OA² − r². The two tangents from A are equal. Circumscribed quadrilateral: AB + CD = BC + AD. Triangle: area = p × r.\n"
          "Regular polygon with n sides: central angle 360° ÷ n; angle (n − 2) × 180° ÷ n; the hexagon has side = r."},
         {"Lengths and areas",
          "L = 2πr    A = πr²\n"
          "Arc: (n/360) × 2πr    Sector: (n/360) × πr²\n"
          "Ring: π(R² − r²)\n"
          "What to do: 1. Is a length (cm) or an area (cm²) asked? 2. Radius or diameter? 3. Keep π; round only at the end. 4. Write the units.\n"
          "Typical slips: central with inscribed angle, the whole chord instead of half, arc length with sector area."},
     },
     {
         {"Example: arc and angle",
          "An inscribed angle BAC = 40° in a circle of radius 9 cm. Find the length of the arc BC.\n"
          "Arc BC = 2 × 40° = 80°.\n"
          "Length = (80/360) × 2π × 9 = (2/9) × 18π = 4π cm."},
         {"Example: chord and tangent",
          "A circle has radius 10 cm. (a) A chord is 6 cm from the centre: how long is it? (b) A tangent is drawn from A, with OA = 26 cm: how long is it?\n"
          "(a) half = √(10² − 6²) = √64 = 8, chord = 2 × 8 = 16 cm.\n"
          "(b) 26² − 10² = 676 − 100 = 576, tangent = 24 cm."},
         {"Example: the polygon",
          "A regular polygon inscribed in a circle has a central angle of 30°. Find n and the angle of the polygon.\n"
          "n = 360° ÷ 30° = 12.\n"
          "Angle = (12 − 2) × 180° ÷ 12 = 1800° ÷ 12 = 150°."},
     }},
// Unit 6 lessons in English, one LessonText per slot (15..20): 6.1 to 6.6. Included by lessons_en.h after Unit 5.
    // slot 15: 6.1
    {"Proportional segments",
     "Geometry",
     {
         {"The ratio of two segments",
          "The ratio of two segments is the quotient of their lengths, both in the same unit: AB/CD. It is a number without a unit, and it does not depend on the unit you choose.\n"
          "In the figure, AM = 6 cm and MB = 4 cm, so AM/MB = 6/4 = 3/2.\n"
          "Careful: for AB = 3 cm and CD = 2 dm, first convert: 2 dm = 20 cm, so AB/CD = 3/20."},
         {"Proportional segments",
          "Four segments are proportional if their lengths (in the same unit) form a proportion: AB/CD = MN/PQ. Check it with the cross product: AB × PQ = CD × MN.\n"
          "A chain of equal ratios, A1B1/C1D1 = A2B2/C2D2 = ... = k, has the proportionality coefficient k.\n"
          "Example: 2/3 = 6/9, because 2 × 9 = 3 × 6 = 18."},
         {"A point on a segment",
          "The point M is on segment AB and AM/MB = a/b. Then AB is split into a + b equal parts, and AM takes a of them:\n"
          "AM = AB × a ÷ (a + b) and MB = AB × b ÷ (a + b).\n"
          "Also AM/AB = a/(a + b), and AM + MB = AB.\n"
          "Careful: AM/MB compares part to part, AM/AB compares part to whole. Do not mix them up."},
         {"Equidistant parallels",
          "Equidistant parallels are parallel lines at equal distances from each other. If they cut one transversal into congruent segments, they cut every transversal into congruent segments.\n"
          "This divides a segment into parts proportional to the numbers a, b, c: add a + b + c, then each part is the length × its number ÷ the sum.\n"
          "Example: 10 cm in the ratio 2 : 3 gives 4 cm and 6 cm."},
     },
     {
         {"Example: the ratio",
          "Segment AB is 6 cm and segment CD is 0.4 dm. What is AB/CD?\n"
          "The units differ, so convert: 0.4 dm = 4 cm.\n"
          "AB/CD = 6/4 = 3/2.\n"
          "The ratio has no unit: AB/CD = 3/2."},
         {"Example: a point on a segment",
          "M is on segment AB = 20 cm and AM/MB = 3/2. Find AM and MB.\n"
          "AB is split into 3 + 2 = 5 parts.\n"
          "AM = 20 × 3 ÷ 5 = 12 cm.\n"
          "MB = 20 − 12 = 8 cm.\n"
          "Check: 12/8 = 3/2."},
         {"Example: proportional parts",
          "Divide a segment of 18 cm into parts proportional to 2, 3 and 4.\n"
          "The sum is 2 + 3 + 4 = 9, so one part is 18 ÷ 9 = 2 cm.\n"
          "The parts are 2 × 2 = 4 cm, 3 × 2 = 6 cm and 4 × 2 = 8 cm.\n"
          "Check: 4 + 6 + 8 = 18 cm. The parts are 4 cm, 6 cm and 8 cm."},
     }},
    // slot 16: 6.2
    {"Thales's theorem",
     "Geometry",
     {
         {"The theorem",
          "A parallel to a side of a triangle cuts the other two sides into proportional segments.\n"
          "In the figure, D is on AB, E is on AC and DE is parallel to BC. Then AD/DB = AE/EC.\n"
          "(The textbook also treats D and E on the extensions of the sides; here they are on the sides.)\n"
          "To use it: write the proportion, then the cross product gives the unknown segment."},
         {"The proportions",
          "From AD/DB = AE/EC the derived proportions follow: AD/AB = AE/AC and DB/AB = EC/AC.\n"
          "Also AB = AD + DB and AC = AE + EC.\n"
          "Careful: pair part with part (AD with DB, AE with EC) or part with whole (AD with AB, AE with AC), the same on both sides. AD/DB = AE/AC is wrong."},
         {"The X figure",
          "In the figure, the lines AB and CD cross at O, and AC is parallel to BD. Thales's theorem gives OA/OB = OC/OD.\n"
          "Each ratio compares the two parts of one line through O, in the same order: OA with OB, OC with OD.\n"
          "Careful: OA/OB = OD/OC is wrong. Check the order of the letters."},
         {"The angle bisector theorem",
          "In the figure, AD is the bisector of angle A, and D is on BC. The bisector divides the opposite side into segments proportional to the other two sides: BD/DC = AB/AC.\n"
          "So BD = BC × AB ÷ (AB + AC) and DC = BC × AC ÷ (AB + AC).\n"
          "Careful: BD goes with AB, DC goes with AC. Check: BD + DC = BC."},
     },
     {
         {"Example: Thales",
          "DE is parallel to BC, with D on AB and E on AC. AD = 4 cm, DB = 6 cm and AE = 6 cm. Find EC and AC.\n"
          "AD/DB = AE/EC, so 4/6 = 6/EC.\n"
          "EC = 6 × 6 ÷ 4 = 9 cm.\n"
          "AC = 6 + 9 = 15 cm."},
         {"Example: the X figure",
          "Lines AB and CD cross at O, and AC is parallel to BD. OA = 4 cm, OB = 6 cm and OC = 10 cm. Find OD.\n"
          "OA/OB = OC/OD, so 4/6 = 10/OD.\n"
          "OD = 6 × 10 ÷ 4 = 15 cm.\n"
          "Check: 4/6 = 2/3 and 10/15 = 2/3."},
         {"Example: the bisector",
          "In triangle ABC, AB = 12 cm, AC = 18 cm and BC = 20 cm. The bisector AD has D on BC. Find BD and DC.\n"
          "BD/DC = AB/AC = 12/18 = 2/3.\n"
          "BD = 20 × 12 ÷ 30 = 8 cm.\n"
          "DC = 20 − 8 = 12 cm. Check: 8/12 = 2/3."},
     }},
    // slot 17: 6.3
    {"Converse of Thales",
     "Geometry",
     {
         {"The statement",
          "In the figure, D is on AB and E is on AC: corresponding positions, both on the sides.\n"
          "Converse of Thales's theorem: if D and E are in corresponding positions and AD/DB = AE/EC, then DE is parallel to BC.\n"
          "So you can prove that two lines are parallel using only lengths."},
         {"How to check",
          "Compute the ratios of the same segments on both sides: AD/AB and AE/AC, or AD/DB and AE/EC. First put all lengths in the same unit.\n"
          "Equal ratios: DE is parallel to BC. Different ratios: DE is not parallel to BC. There is no \"maybe\".\n"
          "Simplify the fractions to compare them: 6/15 = 2/5."},
         {"Common mix-ups",
          "Part to whole on one side and part to part on the other cannot be compared: AD/AB and AE/EC is a mistake.\n"
          "Compare the same segments, in the same order: AD with AE, AB with AC.\n"
          "Different units: 1.5 dm and 6 cm are not 1.5 and 6. Convert first.\n"
          "Do not conclude from a calculator's rounded decimals; compare exact fractions."},
     },
     {
         {"Example: parallel",
          "AB = 12 cm, AD = 4 cm, AC = 15 cm and AE = 5 cm, with D on AB and E on AC. Is DE parallel to BC?\n"
          "AD/AB = 4/12 = 1/3.\n"
          "AE/AC = 5/15 = 1/3.\n"
          "The ratios are equal, so DE is parallel to BC."},
         {"Example: not parallel",
          "AD = 3 cm, DB = 6 cm, AE = 4 cm and EC = 7 cm. Is DE parallel to BC?\n"
          "AD/DB = 3/6 = 1/2.\n"
          "AE/EC = 4/7.\n"
          "1/2 is not 4/7, so DE is not parallel to BC."},
         {"Example: different units",
          "AB = 1.5 dm, AD = 6 cm, AC = 20 cm and AE = 8 cm. Is DE parallel to BC?\n"
          "1.5 dm = 15 cm.\n"
          "AD/AB = 6/15 = 2/5 and AE/AC = 8/20 = 2/5.\n"
          "The ratios are equal, so DE is parallel to BC."},
     }},
    // slot 18: 6.4
    {"Similar triangles",
     "Geometry",
     {
         {"Definition",
          "Triangles ABC and DEF are similar if their angles are equal two by two AND their sides are proportional: A = D, B = E, C = F and AB/DE = AC/DF = BC/EF = k.\n"
          "The order of the letters gives the corresponding vertices: A with D, B with E, C with F. The number k is the similarity ratio.\n"
          "Careful: \"ABC similar to DEF\" and \"ABC similar to FDE\" are different statements."},
         {"The fundamental theorem",
          "T.F.A.: a parallel to a side of a triangle forms a triangle similar to the given one. In the figure, DE is parallel to BC, so triangle ADE is similar to triangle ABC.\n"
          "Therefore AD/AB = AE/AC = DE/BC.\n"
          "Same hypothesis as Thales's theorem, but the conclusion covers all the sides, including DE and BC. DE corresponds to BC, not to DB."},
         {"Finding a side",
          "Write the equal ratios in the order of the letters, then use the cross product.\n"
          "If triangle ABC is similar to triangle DEF, with AB = 6, DE = 4 and BC = 9, then 6/4 = 9/EF, so EF = 9 × 4 ÷ 6 = 6.\n"
          "Careful: pair the corresponding sides, and use one unit for all the lengths."},
         {"Special cases",
          "All equilateral triangles are similar: all their angles are 60°, and the ratio of the sides is the same for each pair.\n"
          "If k = 1, the corresponding sides are equal, so the triangles are congruent: congruence is a special case of similarity.\n"
          "Similar triangles may differ in size, never in shape."},
     },
     {
         {"Example: sides",
          "Triangle ABC is similar to triangle DEF. AB = 6 cm, BC = 9 cm, AC = 12 cm and DE = 4 cm. Find EF and DF.\n"
          "AB/DE = 6/4 = 3/2, and BC/EF = AC/DF = 3/2.\n"
          "EF = 9 × 4 ÷ 6 = 6 cm.\n"
          "DF = 12 × 4 ÷ 6 = 8 cm."},
         {"Example: the T.F.A.",
          "DE is parallel to BC, with D on AB and E on AC. AD = 3 cm, AB = 9 cm, BC = 15 cm and AC = 12 cm. Find DE and AE.\n"
          "Triangle ADE is similar to triangle ABC, so AD/AB = DE/BC = AE/AC = 3/9.\n"
          "DE = 3 × 15 ÷ 9 = 5 cm.\n"
          "AE = 3 × 12 ÷ 9 = 4 cm."},
         {"Example: angles",
          "Triangle ABC is similar to triangle DEF. Angle A = 50° and angle B = 60°. Find angle F, and the side that corresponds to EF.\n"
          "C = 180° − (50° + 60°) = 70°.\n"
          "F corresponds to C, so F = 70°.\n"
          "EF is between E and F, which correspond to B and C, so EF corresponds to BC."},
     }},
    // slot 19: 6.5
    {"Similarity criteria",
     "Geometry",
     {
         {"AA (UU)",
          "AA criterion: if two angles of one triangle are equal to two angles of another, the triangles are similar. The third angles are equal too (180° minus the other two).\n"
          "If A = D and B = E, then triangle ABC is similar to triangle DEF.\n"
          "Careful: the equal angles must correspond (A with D, B with E)."},
         {"SAS (LUL)",
          "SAS criterion: if two sides of one triangle are proportional to two sides of another, and the angles BETWEEN them are equal, the triangles are similar.\n"
          "If AB/DE = AC/DF and A = D, then triangle ABC is similar to triangle DEF.\n"
          "Careful: the angle must be between the two proportional sides. An angle that is not between them proves nothing."},
         {"SSS (LLL)",
          "SSS criterion: if the three sides of one triangle are proportional to the three sides of another, the triangles are similar: AB/DE = AC/DF = BC/EF.\n"
          "Pair the sides by size: the shortest with the shortest, the longest with the longest.\n"
          "Use one unit, and compare exact fractions."},
         {"Heights, perimeters, areas",
          "Two similar triangles with similarity ratio k have:\n"
          "corresponding heights in the ratio k,\n"
          "perimeters in the ratio k,\n"
          "areas in the ratio k².\n"
          "So k = 3 means perimeters 3 times larger but areas 9 times larger. From the ratio of the areas, k is its square root. Careful: the area ratio is k², not k."},
         {"Applications",
          "Similar triangles measure what you cannot reach. At the same moment, a pole and a tree stand upright and the sun's rays are parallel, so they form similar triangles with their shadows.\n"
          "Pole 2 m, shadow 3 m. Tree shadow 12 m. Then height/2 = 12/3, so the height is 8 m.\n"
          "Always: find the equal angles, name the corresponding sides, write the proportion."},
     },
     {
         {"Example: AA",
          "Triangle ABC has A = 45° and B = 75°. Triangle DEF has D = 45° and F = 60°. Are they similar?\n"
          "C = 180° − (45° + 75°) = 60°.\n"
          "E = 180° − (45° + 60°) = 75°.\n"
          "A = D and B = E, so by AA triangle ABC is similar to triangle DEF."},
         {"Example: SAS",
          "AB = 4 cm, AC = 6 cm and angle A = 40° in triangle ABC. DE = 8 cm, DF = 12 cm and angle D = 40°. BC = 5 cm. Find EF.\n"
          "AB/DE = 4/8 = 1/2 and AC/DF = 6/12 = 1/2, and A = D is between them.\n"
          "By SAS the triangles are similar, with k = 1/2.\n"
          "EF = 5 × 2 = 10 cm."},
         {"Example: perimeter and area",
          "Triangle DEF is similar to triangle ABC with ratio 3 (each side of DEF is 3 times the side of ABC). ABC has perimeter 12 cm and area 10 cm².\n"
          "Perimeter: 12 × 3 = 36 cm.\n"
          "Area: 10 × 3² = 10 × 9 = 90 cm².\n"
          "The perimeter of DEF is 36 cm and the area is 90 cm²."},
     }},
    // slot 20: 6.6
    {"Unit 6 review",
     "Geometry",
     {
         {"Which theorem?",
          "DE parallel to BC, D on AB, E on AC: parts of the sides, AD/DB = AE/EC (Thales). If DE or BC is involved: AD/AB = AE/AC = DE/BC (T.F.A.).\n"
          "Two lines crossing at O, AC parallel to BD: OA/OB = OC/OD.\n"
          "Bisector AD: BD/DC = AB/AC.\n"
          "To prove parallel lines: the converse, comparing ratios of the same segments.\n"
          "Point M on AB with AM/MB = a/b: AM = AB × a ÷ (a + b)."},
         {"Similar triangles",
          "To prove similarity: AA (two equal angles), SAS (two proportional sides and the angle between them) or SSS (three proportional sides).\n"
          "Then the corresponding sides are proportional, in the order of the letters, with ratio k.\n"
          "Corresponding heights: ratio k. Perimeters: ratio k. Areas: ratio k²."},
         {"Common mistakes",
          "Same unit before any ratio.\n"
          "Parts and whole: AD/DB is not AD/AB.\n"
          "Corresponding vertices by the order of the letters.\n"
          "For SAS, the angle is between the two sides.\n"
          "The area ratio is k², and k is its square root.\n"
          "Different ratios mean the lines are not parallel."},
     },
     {
         {"Example: Thales and T.F.A.",
          "DE is parallel to BC, with D on AB and E on AC. AD = 2 cm, DB = 3 cm, AE = 4 cm and BC = 15 cm. Find EC and DE.\n"
          "AD/DB = AE/EC, so EC = 3 × 4 ÷ 2 = 6 cm.\n"
          "AB = 2 + 3 = 5 cm, so DE/BC = AD/AB = 2/5.\n"
          "DE = 15 × 2 ÷ 5 = 6 cm."},
         {"Example: the converse",
          "AD = 2 cm, AB = 6 cm, AE = 3 cm and AC = 10 cm, with D on AB and E on AC. Is DE parallel to BC?\n"
          "AD/AB = 2/6 = 1/3.\n"
          "AE/AC = 3/10.\n"
          "1/3 is not 3/10, so DE is not parallel to BC."},
         {"Example: SSS and areas",
          "Triangle ABC has sides 3 cm, 4 cm and 5 cm, and triangle DEF has sides 9 cm, 12 cm and 15 cm. Are they similar? Compare the areas.\n"
          "3/9 = 4/12 = 5/15 = 1/3, so by SSS they are similar.\n"
          "Area of ABC: (3 × 4) ÷ 2 = 6 cm². With k = 3: 6 × 9 = 54 cm².\n"
          "Check: (9 × 12) ÷ 2 = 54 cm²."},
     }},
