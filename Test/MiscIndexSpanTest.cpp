import PotatoMisc;
import std;

// Checks IndexSpan::IsOverlap / IndexSpan::IsDisjoint / IndexSpan::IsEmpty for all nine
// IndexSpanOrdering relations, always comparing the fixed left operand [2, 5) with a shifted
// right operand. Ranges are half-open, so adjacent ranges (End == other.Begin) must not overlap,
// and an empty range (Begin == End) must not overlap anything.

using namespace Potato::Misc;

static int Failed = 0;

static void Check(char const* Name, bool Actual, bool Expected)
{
	if (Actual != Expected)
	{
		++Failed;
		std::cout << "FAILED: " << Name << " actual=" << Actual << " expected=" << Expected << "\n";
	}
}

template<typename T>
static void CheckCase(IndexSpanOrdering ExpectedOrdering, bool ExpectedOverlap)
{
	auto A = IndexSpan<>(2, 5);
	auto B = IndexSpan<>(T::BeginValue, T::EndValue);
	auto Ordering = A <=> B;
	Check(T::Name, static_cast<int>(Ordering) == static_cast<int>(ExpectedOrdering), true);
	Check(T::Name, A.IsOverlap(B), ExpectedOverlap);
	Check(T::Name, A.IsDisjoint(B), !ExpectedOverlap);
	Check(T::Name, IsOverlap(ExpectedOrdering), ExpectedOverlap);
	Check(T::Name, !A.IsEmpty(), true);
}

// The IndexSpanOrdering relation of an empty range may read as Contained, Contains, Equal or Adjacent,
// but an empty range holds no index, so IsOverlap must still report no intersection.
static void CheckEmpty(char const* Name, IndexSpan<> A, IndexSpan<> B)
{
	Check(Name, A.IsEmpty() || B.IsEmpty(), true);
	Check(Name, A.IsOverlap(B), false);
	Check(Name, B.IsOverlap(A), false);
	Check(Name, A.IsDisjoint(B), true);
	Check(Name, B.IsDisjoint(A), true);
}

struct C01 { static constexpr char const* Name = "[0,1) RightDisjoint"; static constexpr std::size_t BeginValue = 0, EndValue = 1; };
struct C02 { static constexpr char const* Name = "[0,2) RightAdjacent"; static constexpr std::size_t BeginValue = 0, EndValue = 2; };
struct C03 { static constexpr char const* Name = "[0,3) RightOverlapEnd"; static constexpr std::size_t BeginValue = 0, EndValue = 3; };
struct C04 { static constexpr char const* Name = "[0,10) Contained"; static constexpr std::size_t BeginValue = 0, EndValue = 10; };
struct C05 { static constexpr char const* Name = "[4,10) LeftOverlapBegin"; static constexpr std::size_t BeginValue = 4, EndValue = 10; };
struct C06 { static constexpr char const* Name = "[5,8) LeftAdjacent"; static constexpr std::size_t BeginValue = 5, EndValue = 8; };
struct C07 { static constexpr char const* Name = "[3,5) Contains"; static constexpr std::size_t BeginValue = 3, EndValue = 5; };
struct C08 { static constexpr char const* Name = "[2,5) Equal"; static constexpr std::size_t BeginValue = 2, EndValue = 5; };
struct C09 { static constexpr char const* Name = "[1,2) RightAdjacent"; static constexpr std::size_t BeginValue = 1, EndValue = 2; };
struct C10 { static constexpr char const* Name = "[6,9) LeftDisjoint"; static constexpr std::size_t BeginValue = 6, EndValue = 9; };
struct C11 { static constexpr char const* Name = "[3,4) Contains"; static constexpr std::size_t BeginValue = 3, EndValue = 4; };
struct C12 { static constexpr char const* Name = "[2,9) Contained"; static constexpr std::size_t BeginValue = 2, EndValue = 9; };
struct C13 { static constexpr char const* Name = "[2,4) Contains"; static constexpr std::size_t BeginValue = 2, EndValue = 4; };
struct C14 { static constexpr char const* Name = "[4,5) Contains"; static constexpr std::size_t BeginValue = 4, EndValue = 5; };

constexpr bool CompileTimeCheck()
{
	IndexSpan<> A{ 2, 5 };
	IndexSpan<> B{ 4, 10 };
	IndexSpan<> C{ 5, 8 };
	IndexSpan<> D{ 3, 4 };
	IndexSpan<> E{ 5 };	// single argument constructor: [5, 6)
	IndexSpan<> Empty1{ 5, 5 };
	IndexSpan<> Empty2{ 5, 5 };
	IndexSpan<> Empty3{};	// [0, 0)
	IndexSpan<> Empty4{ 100, 100 };
	return A.IsOverlap(B) && !A.IsOverlap(C) && A.IsOverlap(D) && !A.IsOverlap(E) && A.IsOverlap(IndexSpan<>{ 4, 6 })
		&& E.Begin() == 5 && E.End() == 6
		// Empty ranges never overlap, whatever their IndexSpanOrdering says.
		&& !Empty1.IsOverlap(A) && !A.IsOverlap(Empty1) && !Empty2.IsOverlap(Empty1)
		&& !Empty3.IsOverlap(A) && !Empty4.IsOverlap(Empty3) && !Empty4.IsOverlap(A)
		&& Empty1.IsDisjoint(A) && Empty4.IsEmpty() && !A.IsEmpty()
		&& IsOverlap(IndexSpanOrdering::Equal) && !IsOverlap(IndexSpanOrdering::LeftAdjacent);
}
static_assert(CompileTimeCheck(), "IndexSpan overlap must work at compile time");

int main()
{
	CheckCase<C01>(IndexSpanOrdering::RightDisjoint, false);
	CheckCase<C02>(IndexSpanOrdering::RightAdjacent, false);
	CheckCase<C03>(IndexSpanOrdering::RightOverlapEnd, true);
	CheckCase<C04>(IndexSpanOrdering::Contained, true);
	CheckCase<C05>(IndexSpanOrdering::LeftOverlapBegin, true);
	CheckCase<C06>(IndexSpanOrdering::LeftAdjacent, false);
	CheckCase<C07>(IndexSpanOrdering::Contains, true);
	CheckCase<C08>(IndexSpanOrdering::Equal, true);
	CheckCase<C09>(IndexSpanOrdering::RightAdjacent, false);
	CheckCase<C10>(IndexSpanOrdering::LeftDisjoint, false);
	CheckCase<C11>(IndexSpanOrdering::Contains, true);
	CheckCase<C12>(IndexSpanOrdering::Contained, true);
	CheckCase<C13>(IndexSpanOrdering::Contains, true);
	CheckCase<C14>(IndexSpanOrdering::Contains, true);

	// An empty range keeps its IndexSpanOrdering relation but must never overlap.
	CheckEmpty("[5,5) vs [2,5)", IndexSpan<>{5, 5}, IndexSpan<>{2, 5});
	CheckEmpty("[2,5) vs [5,5)", IndexSpan<>{2, 5}, IndexSpan<>{5, 5});
	CheckEmpty("[5,5) vs [5,5)", IndexSpan<>{5, 5}, IndexSpan<>{5, 5});
	CheckEmpty("[0,0) vs [0,10)", IndexSpan<>{0, 0}, IndexSpan<>{0, 10});
	CheckEmpty("[4,4) vs [0,10)", IndexSpan<>{4, 4}, IndexSpan<>{0, 10});
	CheckEmpty("[0,10) vs [7,7)", IndexSpan<>{0, 10}, IndexSpan<>{7, 7});
	CheckEmpty("[100,100) vs [5,8)", IndexSpan<>{100, 100}, IndexSpan<>{5, 8});

	if (Failed == 0)
		std::cout << "IndexSpan overlap check pass!\n";
	else
		std::cout << Failed << " check(s) failed\n";
	return Failed == 0 ? 0 : 1;
}
