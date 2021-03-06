#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <inttypes.h>
#include <cassert>
#include <cstdlib>
#include <sstream>
#include <cmath>
#include <random>
#include <time.h>
using namespace std;
typedef unsigned int uint;
typedef unsigned char uint8;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef signed int sint;
#define trace(exp) cerr << __LINE__ << ": " << #exp << " = \"" << (exp) << "\"" << endl;

bool debug = false;

inline uint popcount(uint64 n)
{
	return __builtin_popcountll(n);
}

inline uint trailingZeros(uint64 n)
{
	return __builtin_ctzll(n);
}

uint32 entropy(uint upperBound)
{
	static std::mt19937_64 engine(time(0));
	const static uint64 full = (engine.max() - engine.min()) >> 8; // 8 bit safety margin
	static uint64 entropy = 0;
	static uint64 available = 0;
	
	// Fill entropy pool
	if(available < upperBound) {
		entropy = engine() - engine.min();
		available = full;
	}
	
	// Take entropy out
	uint dice = entropy % upperBound;
	entropy /= upperBound;
	available /= upperBound;
	return dice;
}

// true = white
bool blackOrWhite(uint numBlack, uint numWhite)
{
	// Short cut if there is no entropy involved
	if(numBlack == 0)
		return true;
	if(numWhite == 0)
		return false;
	
	// We can divide out gcd(numBlack, numWhite) , let's only do it for powers of two
	//uint ctz = trailingZeros(numBlack | numWhite);
	//numBlack >>= ctz;
	//numWhite >>= ctz;
	uint orred = numBlack | numWhite;
	while((orred & 1) == 0) {
		orred >>= 1;
		numBlack >>= 1;
		numWhite >>= 1;
	}
	
	return entropy(numBlack + numWhite) < numWhite;
}

inline float randomReal()
{
	return static_cast<float>(entropy(1000)) / 1000.0;
}

class Timer {
public:
	static Timer instance;
	Timer(uint timeLimit, uint expectedRounds, uint maxRounds);
	~Timer() { }
	
	void nextRound();
	bool ponder();
	
protected:
	uint _timeLimit;
	uint _currentRound;
	uint _expectedRounds;
	uint _maxRounds;
	uint _roundLimit;
	uint _endRoundLimit;
	clock_t _roundStart;
};

// Most games last 30-60 moves -> spend most time on the fist 25 moves per player out of max 53
#ifdef LOCAL
Timer Timer::instance(11, 25, 53); // Equivalent in cpu power to 
#else
Timer Timer::instance(29, 25, 53);
#endif

Timer::Timer(uint timeLimit, uint expectedRounds,  uint maxRounds)
: _timeLimit(timeLimit)
, _currentRound(0)
, _expectedRounds(expectedRounds)
, _maxRounds(maxRounds)
, _roundLimit(0)
, _roundStart()
{
	_roundLimit = (_timeLimit * 900) / _expectedRounds;
	_endRoundLimit = (_timeLimit * 100) / (_maxRounds - _expectedRounds);
}

void Timer::nextRound()
{
	++_currentRound;
	_roundStart = clock();
}

bool Timer::ponder()
{
	clock_t now = clock();
	uint duration = (1000 * (now - _roundStart)) / CLOCKS_PER_SEC;
	return duration < ((_currentRound < _expectedRounds) ? _roundLimit : _endRoundLimit);
}

class BoardPoint;
class BoardMask;
class Board;
class Move;
class TreeNode;

class BoardPoint {
public:
	static constexpr uint maxRotation = 10;
	static constexpr uint maxIndex = 105;
	static constexpr uint numIndices = 106;
	static BoardPoint fromIndex(uint index) { return BoardPoint(index + 1); }
	
	BoardPoint(): _position(0) { }
	BoardPoint(uint position): _position(position) { }
	~BoardPoint() { }
	
	uint index() const { return _position - 1; }
	bool operator!=(const BoardPoint& other) const { return _position != other._position; }
	bool operator==(const BoardPoint& other) const { return _position == other._position; }
	bool isValid() const { return _position >= 1 && _position <= 106; }
	uint position() const { return _position; }
	BoardMask mask() const;
	BoardPoint& rotate(uint degrees) { *this = rotated(degrees); return *this; }
	BoardPoint rotated(uint degrees) const { return BoardPoint(_rotations[degrees][index()]); }
	BoardMask neighbors() const;
	BoardPoint& position(uint value) { _position = value; return *this; }
	
protected:
	static const BoardMask _neighbors[106];
	static const uint8 _rotations[10][106];
	uint _position; /// [1...106] inclusive
};

const uint8 BoardPoint::_rotations[10][106] = {
	{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106},
	{49,61,48,36,72,60,47,35,25,82,71,59,46,34,24,16,91,81,70,58,45,33,23,15,9,99,90,80,69,57,44,32,22,14,8,4,106,98,89,79,68,56,43,31,21,13,7,3,1,105,97,88,78,67,55,42,30,20,12,6,2,104,96,87,77,66,54,41,29,19,11,5,103,95,86,76,65,53,40,28,18,10,102,94,85,75,64,52,39,27,17,101,93,84,74,63,51,38,26,100,92,83,73,62,50,37},
	{37,50,38,26,62,51,39,27,17,73,63,52,40,28,18,10,83,74,64,53,41,29,19,11,5,92,84,75,65,54,42,30,20,12,6,2,100,93,85,76,66,55,43,31,21,13,7,3,1,101,94,86,77,67,56,44,32,22,14,8,4,102,95,87,78,68,57,45,33,23,15,9,103,96,88,79,69,58,46,34,24,16,104,97,89,80,70,59,47,35,25,105,98,90,81,71,60,48,36,106,99,91,82,72,61,49},
	{106,105,98,99,104,97,89,90,91,103,96,88,79,80,81,82,102,95,87,78,68,69,70,71,72,101,94,86,77,67,56,57,58,59,60,61,100,93,85,76,66,55,43,44,45,46,47,48,49,92,84,75,65,54,42,31,32,33,34,35,36,83,74,64,53,41,30,21,22,23,24,25,73,63,52,40,29,20,13,14,15,16,62,51,39,28,19,12,7,8,9,50,38,27,18,11,6,3,4,37,26,17,10,5,2,1},
	{100,101,93,92,102,94,85,84,83,103,95,86,76,75,74,73,104,96,87,77,66,65,64,63,62,105,97,88,78,67,55,54,53,52,51,50,106,98,89,79,68,56,43,42,41,40,39,38,37,99,90,80,69,57,44,31,30,29,28,27,26,91,81,70,58,45,32,21,20,19,18,17,82,71,59,46,33,22,13,12,11,10,72,60,47,34,23,14,7,6,5,61,48,35,24,15,8,3,2,49,36,25,16,9,4,1},
	{100,92,93,101,83,84,85,94,102,73,74,75,76,86,95,103,62,63,64,65,66,77,87,96,104,50,51,52,53,54,55,67,78,88,97,105,37,38,39,40,41,42,43,56,68,79,89,98,106,26,27,28,29,30,31,44,57,69,80,90,99,17,18,19,20,21,32,45,58,70,81,91,10,11,12,13,22,33,46,59,71,82,5,6,7,14,23,34,47,60,72,2,3,8,15,24,35,48,61,1,4,9,16,25,36,49},
	{106,99,98,105,91,90,89,97,104,82,81,80,79,88,96,103,72,71,70,69,68,78,87,95,102,61,60,59,58,57,56,67,77,86,94,101,49,48,47,46,45,44,43,55,66,76,85,93,100,36,35,34,33,32,31,42,54,65,75,84,92,25,24,23,22,21,30,41,53,64,74,83,16,15,14,13,20,29,40,52,63,73,9,8,7,12,19,28,39,51,62,4,3,6,11,18,27,38,50,1,2,5,10,17,26,37},
	{37,26,38,50,17,27,39,51,62,10,18,28,40,52,63,73,5,11,19,29,41,53,64,74,83,2,6,12,20,30,42,54,65,75,84,92,1,3,7,13,21,31,43,55,66,76,85,93,100,4,8,14,22,32,44,56,67,77,86,94,101,9,15,23,33,45,57,68,78,87,95,102,16,24,34,46,58,69,79,88,96,103,25,35,47,59,70,80,89,97,104,36,48,60,71,81,90,98,105,49,61,72,82,91,99,106},
	{49,36,48,61,25,35,47,60,72,16,24,34,46,59,71,82,9,15,23,33,45,58,70,81,91,4,8,14,22,32,44,57,69,80,90,99,1,3,7,13,21,31,43,56,68,79,89,98,106,2,6,12,20,30,42,55,67,78,88,97,105,5,11,19,29,41,54,66,77,87,96,104,10,18,28,40,53,65,76,86,95,103,17,27,39,52,64,75,85,94,102,26,38,51,63,74,84,93,101,37,50,62,73,83,92,100},
	{1,4,3,2,9,8,7,6,5,16,15,14,13,12,11,10,25,24,23,22,21,20,19,18,17,36,35,34,33,32,31,30,29,28,27,26,49,48,47,46,45,44,43,42,41,40,39,38,37,61,60,59,58,57,56,55,54,53,52,51,50,72,71,70,69,68,67,66,65,64,63,62,82,81,80,79,78,77,76,75,74,73,91,90,89,88,87,86,85,84,83,99,98,97,96,95,94,93,92,106,105,104,103,102,101,100}
};


class Move: public BoardPoint {
public:
	static constexpr uint maxIndex = 106;
	static constexpr uint numIndices = 107;
	static Move fromIndex(uint index) { return Move(index + 1); }
	static Move Swap;
	
	Move(): BoardPoint(0) { }
	Move(const BoardPoint& point): BoardPoint(point) { }
	explicit Move(const string& str);
	Move(uint position): BoardPoint(position) { }
	~Move() { }
	
	bool isValid() const { return BoardPoint::isValid() || (*this == Swap); }
	
protected:
	static const BoardMask _neighbors[106];
};

Move Move::Swap(107);

std::ostream& operator<<(std::ostream& out, const Move& move)
{
	if(move == Move::Swap)
		out << "-1";
	else
		out << move.position();
	return out;
}

std::istream& operator>>(std::istream& in, Move& move)
{
	int position;
	in >> position;
	move = (position == -1) ? Move::Swap : Move(position);
	assert(move.isValid());
	return in;
}

Move::Move(const string& str)
: BoardPoint(0)
{
	stringstream stream(str);
	stream >> *this;
}

class BoardMask {
public:
	class Iterator;
	const static BoardMask fullBoard;
	const static BoardMask borders[5];
	
	BoardMask(): _a(0), _b(0) { }
	BoardMask(const BoardMask& other): _a(other._a), _b(other._b) { }
	BoardMask(BoardPoint point);
	~BoardMask() { }
	operator bool() const { return !isEmpty(); }
	BoardMask& operator=(const BoardMask& other) { _a = other._a; _b = other._b; return *this; }
	bool operator==(const BoardMask& other) const { return _a == other._a && _b == other._b; }
	bool operator!=(const BoardMask& other) const { return !operator==(other); }
	BoardMask& operator&=(const BoardMask& other) { return operator=(operator&(other)); }
	BoardMask& operator|=(const BoardMask& other) { return operator=(operator|(other)); }
	BoardMask& operator-=(const BoardMask& other) { return operator=(operator-(other)); }
	BoardMask operator&(const BoardMask& other) const { return BoardMask(_a & other._a, _b & other._b); }
	BoardMask operator|(const BoardMask& other) const { return BoardMask(_a | other._a, _b | other._b); }
	BoardMask operator-(const BoardMask& other) const { return *this & (~other); }
	BoardMask operator~() const { return BoardMask(~_a, ~_b) & fullBoard; }
	BoardMask expanded() const;
	BoardMask connected(const BoardMask& seed) const;
	vector<BoardMask> groups() const;
	uint controlledCorners() const;
	BoardMask& invert() { return operator=(operator~()); }
	BoardMask& expand() { return operator=(expanded()); }
	BoardMask& clear() { return operator=(BoardMask()); }
	uint popcount() const { return ::popcount(_a) + ::popcount(_b); }
	bool isSet(BoardPoint point) const { return !(*this & BoardMask(point)).isEmpty(); }
	BoardMask& set(BoardPoint point) { return operator=(*this | BoardMask(point)); }
	BoardMask& clear(BoardPoint point) { return operator=(*this & ~BoardMask(point)); }
	bool isEmpty() const { return operator==(BoardMask()); }
	bool isValid() const { return operator==(*this & fullBoard); }
	BoardPoint firstPoint() const;
	BoardPoint randomPoint() const;
	Iterator iterator() const;
	
	uint64 a() const { return _a; }
	uint64 b() const { return _b; }
	
protected:
	BoardMask(uint64 a, uint64 b) :_a(a), _b(b) { }
	
protected:
	uint64 _a;
	uint64 _b;
};

class BoardMask::Iterator {
public:
	Iterator(const BoardMask& mask): _mask(mask), _point(_mask.firstPoint()) { }
	~Iterator() { }
	operator bool() const { return _mask._a | _mask._b; }
	Iterator& operator++() { _mask.clear(_point); _point = _mask.firstPoint(); return *this; }
	Iterator operator++(int) { Iterator tmp(*this); operator++(); return tmp; }
	const BoardPoint& operator*() const { return _point; }
	const BoardPoint* operator->() const { return &_point; }
	
private:
	BoardMask _mask;
	BoardPoint _point;
};

std::ostream& operator<<(std::ostream& out, const BoardMask& mask)
{
	for(uint i = 1; i <= 106; ++i)
		out << (mask.isSet(i) ? "#" : ".");
	return out;
}

const BoardMask BoardPoint::_neighbors[106] = {
	BoardMask().set(2).set(3).set(4), // 1
	BoardMask().set(1).set(3).set(5).set(6), // 2
	BoardMask().set(1).set(2).set(6).set(7).set(8).set(4), // 3
	BoardMask().set(1).set(3).set(8).set(9), // 4
	BoardMask().set(2).set(6).set(11).set(10), // 5
	BoardMask().set(2).set(5).set(11).set(12).set(7).set(3), // 6
	BoardMask().set(3).set(6).set(12).set(13).set(14).set(8), // 7
	BoardMask().set(4).set(3).set(7).set(14).set(15).set(9), // 8
	BoardMask().set(4).set(8).set(15).set(16), // 9
	BoardMask().set(5).set(11).set(18).set(17), // 10
	BoardMask().set(5).set(10).set(18).set(19).set(12).set(6), // 11
	BoardMask().set(6).set(11).set(19).set(20).set(13).set(7), // 12
	BoardMask().set(7).set(12).set(20).set(21).set(22).set(14), // 13
	BoardMask().set(8).set(7).set(13).set(22).set(23).set(15), // 14
	BoardMask().set(9).set(8).set(14).set(23).set(24).set(16), // 15
	BoardMask().set(9).set(15).set(24).set(25), // 16
	BoardMask().set(10).set(18).set(26).set(27), // 17
	BoardMask().set(10).set(17).set(27).set(28).set(19).set(11), // 18
	BoardMask().set(11).set(18).set(28).set(29).set(20).set(12), // 19
	BoardMask().set(12).set(19).set(29).set(30).set(21).set(13), // 20
	BoardMask().set(13).set(20).set(30).set(31).set(32).set(22), // 21
	BoardMask().set(14).set(13).set(21).set(32).set(33).set(23), // 22
	BoardMask().set(15).set(14).set(22).set(33).set(34).set(24), // 23
	BoardMask().set(16).set(15).set(23).set(34).set(35).set(25), // 24
	BoardMask().set(16).set(24).set(35).set(36), // 25
	BoardMask().set(17).set(27).set(37).set(38), // 26
	BoardMask().set(17).set(26).set(38).set(39).set(28).set(18), // 27
	BoardMask().set(18).set(27).set(39).set(40).set(29).set(19), // 28
	BoardMask().set(19).set(28).set(40).set(41).set(30).set(20), // 29
	BoardMask().set(20).set(29).set(41).set(42).set(31).set(21), // 30
	BoardMask().set(21).set(30).set(42).set(43).set(44).set(32), // 31
	BoardMask().set(22).set(21).set(31).set(44).set(45).set(33), // 32
	BoardMask().set(23).set(22).set(32).set(45).set(46).set(34), // 33
	BoardMask().set(24).set(23).set(33).set(46).set(47).set(35), // 34
	BoardMask().set(25).set(24).set(34).set(47).set(48).set(36), // 35
	BoardMask().set(25).set(35).set(48).set(49), // 36
	BoardMask().set(26).set(38).set(50), // 37
	BoardMask().set(26).set(37).set(50).set(51).set(39).set(27), // 38
	BoardMask().set(27).set(38).set(51).set(52).set(40).set(28), // 39
	BoardMask().set(28).set(39).set(52).set(53).set(41).set(29), // 40
	BoardMask().set(29).set(40).set(53).set(54).set(42).set(30), // 41
	BoardMask().set(30).set(41).set(54).set(55).set(43).set(31), // 42
	BoardMask().set(31).set(42).set(55).set(56).set(44), // 43
	BoardMask().set(31).set(43).set(56).set(57).set(45).set(32), // 44
	BoardMask().set(32).set(44).set(57).set(58).set(46).set(33), // 45
	BoardMask().set(33).set(45).set(58).set(59).set(47).set(34), // 46
	BoardMask().set(34).set(46).set(59).set(60).set(48).set(35), // 47
	BoardMask().set(35).set(47).set(60).set(61).set(49).set(36), // 48
	BoardMask().set(36).set(48).set(61), // 49
	BoardMask().set(37).set(38).set(51).set(62), // 50
	BoardMask().set(38).set(50).set(62).set(63).set(52).set(39), // 51
	BoardMask().set(39).set(51).set(63).set(64).set(53).set(40), // 52
	BoardMask().set(40).set(52).set(64).set(65).set(54).set(41), // 53
	BoardMask().set(41).set(53).set(65).set(66).set(55).set(42), // 54
	BoardMask().set(42).set(54).set(66).set(67).set(56).set(43), // 55
	BoardMask().set(43).set(55).set(67).set(68).set(57).set(44), // 56
	BoardMask().set(44).set(56).set(68).set(69).set(58).set(45), // 57
	BoardMask().set(45).set(57).set(69).set(70).set(59).set(46), // 58
	BoardMask().set(46).set(58).set(70).set(71).set(60).set(47), // 59
	BoardMask().set(47).set(59).set(71).set(72).set(61).set(48), // 60
	BoardMask().set(49).set(48).set(60).set(72), // 61
	BoardMask().set(50).set(51).set(63).set(73), // 62
	BoardMask().set(51).set(62).set(73).set(74).set(64).set(52), // 63
	BoardMask().set(52).set(63).set(74).set(75).set(65).set(53), // 64
	BoardMask().set(53).set(64).set(75).set(76).set(66).set(54), // 65
	BoardMask().set(54).set(65).set(76).set(77).set(67).set(55), // 66
	BoardMask().set(55).set(66).set(77).set(78).set(68).set(56), // 67
	BoardMask().set(56).set(67).set(78).set(79).set(69).set(57), // 68
	BoardMask().set(57).set(68).set(79).set(80).set(70).set(58), // 69
	BoardMask().set(58).set(69).set(80).set(81).set(71).set(59), // 70
	BoardMask().set(59).set(70).set(81).set(82).set(72).set(60), // 71
	BoardMask().set(61).set(60).set(71).set(82), // 72
	BoardMask().set(62).set(63).set(74).set(83), // 73
	BoardMask().set(63).set(73).set(83).set(84).set(75).set(64), // 74
	BoardMask().set(64).set(74).set(84).set(85).set(76).set(65), // 75
	BoardMask().set(65).set(75).set(85).set(86).set(77).set(66), // 76
	BoardMask().set(66).set(76).set(86).set(87).set(78).set(67), // 77
	BoardMask().set(67).set(77).set(87).set(88).set(79).set(68), // 78
	BoardMask().set(68).set(78).set(88).set(89).set(80).set(69), // 79
	BoardMask().set(69).set(79).set(89).set(90).set(81).set(70), // 80
	BoardMask().set(70).set(80).set(90).set(91).set(82).set(71), // 81
	BoardMask().set(72).set(71).set(81).set(91), // 82
	BoardMask().set(73).set(74).set(84).set(92), // 83
	BoardMask().set(74).set(83).set(92).set(93).set(85).set(75), // 84
	BoardMask().set(75).set(84).set(93).set(94).set(86).set(76), // 85
	BoardMask().set(76).set(85).set(94).set(95).set(87).set(77), // 86
	BoardMask().set(77).set(86).set(95).set(96).set(88).set(78), // 87
	BoardMask().set(78).set(87).set(96).set(97).set(89).set(79), // 88
	BoardMask().set(79).set(88).set(97).set(98).set(90).set(80), // 89
	BoardMask().set(80).set(89).set(98).set(99).set(91).set(81), // 90
	BoardMask().set(82).set(81).set(90).set(99), // 91
	BoardMask().set(83).set(84).set(93).set(100), // 92
	BoardMask().set(84).set(92).set(100).set(101).set(94).set(85), // 93
	BoardMask().set(85).set(93).set(101).set(102).set(95).set(86), // 94
	BoardMask().set(86).set(94).set(102).set(103).set(96).set(87), // 95
	BoardMask().set(87).set(95).set(103).set(104).set(97).set(88), // 96
	BoardMask().set(88).set(96).set(104).set(105).set(98).set(89), // 97
	BoardMask().set(89).set(97).set(105).set(106).set(99).set(90), // 98
	BoardMask().set(91).set(90).set(98).set(106), // 99
	BoardMask().set(92).set(93).set(101), // 100
	BoardMask().set(100).set(93).set(94).set(102), // 101
	BoardMask().set(101).set(94).set(95).set(103), // 102
	BoardMask().set(102).set(95).set(96).set(104), // 103
	BoardMask().set(103).set(96).set(97).set(105), // 104
	BoardMask().set(104).set(97).set(98).set(106), // 105
	BoardMask().set(105).set(98).set(99) // 106
};

const BoardMask BoardMask::fullBoard(0xfffffffffffffffe, 0x7ffffffffff);

const BoardMask BoardMask::borders[5] {
	BoardMask().set(1).set(4).set(9).set(16).set(25).set(36).set(49),
	BoardMask().set(49).set(61).set(72).set(82).set(91).set(99).set(106),
	BoardMask().set(106).set(105).set(104).set(103).set(102).set(101).set(100),
	BoardMask().set(100).set(92).set(83).set(73).set(62).set(50).set(37),
	BoardMask().set(37).set(26).set(17).set(10).set(5).set(2).set(1)
};

inline BoardMask BoardPoint::mask() const
{
	return BoardMask(*this);
}

inline BoardMask BoardPoint::neighbors() const
{
	assert(isValid());
	return _neighbors[position() - 1];
}

inline BoardMask::BoardMask(BoardPoint point)
: _a(point.position() <= 63 ? (1ULL << point.position()) : 0)
, _b(point.position() <= 63 ? 0 : (1ULL << (point.position() - 64)))
{
}

inline BoardMask::Iterator BoardMask::iterator() const
{
	return Iterator(*this);
}

BoardMask BoardMask::expanded() const
{
	BoardMask result(*this);
	for(Iterator i(*this); i; ++i)
		result |= i->neighbors();
	return result;
}

BoardMask BoardMask::connected(const BoardMask& seed) const
{
	assert(isValid() && seed.isValid());
	BoardMask result = *this & seed;
	BoardMask border = result;
	while(border) {
		BoardMask nextBorder;
		for(auto i = border.iterator(); i; ++i)
			nextBorder |= i->neighbors() & *this;
		nextBorder -= result;
		result |= nextBorder;
		border = nextBorder;
	}
	return result;
}

vector<BoardMask> BoardMask::groups() const
{
	vector<BoardMask> result;
	BoardMask copy = *this;
	while(copy) {
		BoardMask group = copy.connected(copy.firstPoint());
		result.push_back(group);
		copy -= group;
	}
	return result;
}

uint BoardMask::controlledCorners() const
{
	uint corners = 0;
	
	// Iterate connected groups
	BoardMask remaining(*this);
	while(remaining) {
		// Find a group
		BoardMask group(remaining.firstPoint());
		group = remaining.connected(group);
		remaining -= group;
		
		// See which borders are connected
		uint borders = 0;
		for(uint i = 0; i < 5; ++i)
			if(group & BoardMask::borders[i])
				borders |= 1 << i;
		
		// If three borders are connected, any adjacent borders have the corner controlled
		if(::popcount(borders) >= 3)
			corners |= borders & ((borders >> 1) | (borders << 4));
	}
	return ::popcount(corners);
}

inline BoardPoint BoardMask::firstPoint() const
{
	if(_a)
		return BoardPoint(trailingZeros(_a));
	else if(_b)
		return BoardPoint(64 + trailingZeros(_b));
	else
		return BoardPoint();
}

BoardPoint BoardMask::randomPoint() const
{
	uint p = popcount();
	if(p == 0)
		return BoardPoint();
	uint n = entropy(p);
	Iterator i(*this);
	while(n--)
		i++;
	return *i;
}

class Board {
public:
	Board(): _white(), _black(), _moveCount(0) { }
	~Board() { }
	bool operator==(const Board& other) const { return _moveCount == other._moveCount && _white == other._white && _black == other._black; }
	
	BoardMask nonSwapMoves() const { return ~(_white | _black); }
	bool gameOver() const;
	void playMove(Move move);
	
	uint moveCount() const { return _moveCount; }
	BoardMask white() const { return _white; }
	BoardMask black() const { return _black; }
	
	uint player() const { return (_moveCount & 1) ? 2 : 1; }
	uint winner() const;
	
	void bambooBridges();
	void randomFillUp();
	
protected:
	BoardMask _white;
	BoardMask _black;
	uint8 _moveCount;
};

std::ostream& operator<<(std::ostream& out, const Board& board)
{
	for(uint i = 1; i <= 106; ++i) {
		bool white = board.white().isSet(i);
		bool black = board.black().isSet(i);
		if(!white && !black)
			out << ".";
		if(white && !black)
			out << "W";
		if(!white && black)
			out << "B";
		if(white && black)
			out << "!";
	}
	return out;
}

void Board::playMove(Move move)
{
	// Increase move counter
	++_moveCount;
	
	// Swap move
	if(move == Move::Swap) {
		assert(_moveCount == 2);
		swap(_black, _white);
		return;
	}
	
	// Placement move
	if(_moveCount & 1)
		_black.set(move);
	else
		_white.set(move);
}

bool Board::gameOver() const
{
	if(!nonSwapMoves())
		return true;
	if(_white.controlledCorners() >= 3)
		return true;
	if(_black.controlledCorners() >= 3)
		return true;
	return false;
}

uint Board::winner() const
{
	if(_white.controlledCorners() >= 3)
		return 1;
	if(_black.controlledCorners() >= 3)
		return 2;
	return 0;
}

// Connects bamboo bridges randomly
void Board::bambooBridges()
{
	BoardMask free = ~(_black | _white);
	
	// Find groups and extensions
	vector<BoardMask> whiteGroups = _white.groups();
	vector<BoardMask> whiteExtensions;
	for(const BoardMask& g: whiteGroups)
		whiteExtensions.push_back(g.expanded());
	vector<BoardMask> blackGroups = _black.groups();
	vector<BoardMask> blackExtensions;
	for(const BoardMask& g: blackGroups)
		blackExtensions.push_back(g.expanded());
	
	// Randomly play black or white
	uint whiteIndex = 0;
	uint blackIndex = 0;
	while((whiteIndex < whiteGroups.size()) || (blackIndex < blackGroups.size())) {
		uint i;
		BoardMask group;
		BoardMask* player = nullptr;
		BoardMask* opponent = nullptr;
		vector<BoardMask>* self = nullptr;
		if(whiteIndex != whiteGroups.size() && (blackIndex == blackGroups.size() || entropy(2))) {
			i = whiteIndex++;
			group = whiteGroups[i];
			player = &_white;
			opponent = &_black;
			self = &whiteExtensions;
		} else {
			i = blackIndex++;
			group = blackGroups[i];
			player = &_black;
			opponent = &_white;
			self = &blackExtensions;
		}
		BoardMask extension = self->at(i);
		
		// Borders
		for(uint j = 0; j < 5; j++) {
			BoardMask border = BoardMask::borders[j];
			if(group & border)
				continue; // Already connected to border
			BoardMask bamboo = extension & border & free;
			if(bamboo.popcount() == 2) {
				BoardPoint p = bamboo.randomPoint();
				player->set(p);
				*opponent |= bamboo - *player;
				free -= bamboo;
			}
		}
		
		// Other groups
		for(uint j = 0; j < i; j++) {
			BoardMask bamboo = extension & self->at(j) & free;
			if(bamboo.popcount() == 2) {
				BoardPoint p = bamboo.randomPoint();
				player->set(p);
				*opponent |= bamboo - *player;
				free -= bamboo;
			}
		}
	}
}

void Board::randomFillUp()
{
	BoardMask free = ~(_black | _white);
	uint whiteStones = 53 - _black.popcount();
	uint blackStones = 53 - _white.popcount();
	
	// Fill up with equal amounts of stones
	for(auto i = free.iterator(); i; i++) {
		if(blackOrWhite(blackStones, whiteStones)) {
			_white.set(*i);
			whiteStones--;
		} else {
			_black.set(*i);
			blackStones--;
		}
	}
	assert(whiteStones == 0 && blackStones == 0);
}

class HistoryHeuristic {
public:
	static HistoryHeuristic player1;
	static HistoryHeuristic player2;
	HistoryHeuristic();
	
	float heuristic(uint moveIndex) const;
	void countWin(const BoardMask& endGame);
	void countLoss(const BoardMask& endGame);
	
private:
	static constexpr float epsilon = 1e-6;
	float _score[Move::numIndices];
	uint _visits[Move::numIndices];
};

HistoryHeuristic HistoryHeuristic::player1;
HistoryHeuristic HistoryHeuristic::player2;

HistoryHeuristic::HistoryHeuristic()
: _score()
, _visits()
{
	for(uint i = 0; i < Move::numIndices; ++i) {
		_score[i] = 0.0;
		_visits[i] = 0;
	}
}

float HistoryHeuristic::heuristic(uint moveIndex) const
{
	return _score[moveIndex] / (float(_visits[moveIndex]) + epsilon);
}

void HistoryHeuristic::countLoss(const BoardMask& endGame)
{
	for(BoardMask::Iterator i = endGame.iterator(); i; ++i)
		_visits[i->index()] += 1;
}

void HistoryHeuristic::countWin(const BoardMask& endGame)
{
	for(BoardMask::Iterator i = endGame.iterator(); i; ++i) {
		_score[i->index()] += 1.0;
		_visits[i->index()] += 1;
	}
}

class TreeNode {
public:
	static constexpr float epsilon = 1e-6;
	static constexpr float explorationParameter = 0.35; // C
	static constexpr float historyParameter = 6.00; // W 
	static uint numNodes() { return _numNodes; }
	
	TreeNode();
	TreeNode(TreeNode* parent, Move move);
	~TreeNode();
	
	Move move() const { return _move; }
	
	TreeNode* child(Move move);
	
	// Favorite child, forget all other children
	void vincent(TreeNode* child);
	
	uint depth() const;
	uint numVisitedChildren() const;
	BoardMask visitedChildren() const;
	
	
	void loadGames(const string& file);
	void read(const string& filename, uint rotation = 0);
	void read(istream& in, uint rotation = 0);
	void write(const string& filename, uint treshold = 0) const;
	void write(ostream& out, uint treshold = 0) const;
	
	void writeOut(ostream& out, uint depth) const;
	
	void scaleStatistics(uint factor);
	void updateStatsUpwards(float score);
	void updateStats(float score);
	
	void selectAction(Board board);
	bool isLeaf() { return _visits == 0; }
	float rollOut(const Board& board) const;
	
	float visits() const { return _visits; }
	float totalScore() const { return _totalScore; }
	
	Move bestMove() const;
	
protected:
	friend ostream& operator<<(ostream& out, const TreeNode& treeNode);
	static uint _numNodes;
	
	Move _move;
	uint _visits;
	float _totalScore;
	TreeNode* _parent;
	TreeNode* _child;
	TreeNode* _sibling;
	TreeNode* select(const Board& board);
};

uint TreeNode::_numNodes = 0;

TreeNode::TreeNode()
: _move()
, _visits(0)
, _totalScore(0.0f)
, _parent(nullptr)
, _child(nullptr)
, _sibling(nullptr)
{
	_numNodes++;
}

TreeNode::TreeNode(TreeNode* parent, Move move)
: _move(move)
, _visits(0)
, _totalScore(0.0f)
, _parent(parent)
, _child(nullptr)
, _sibling(nullptr)
{
	_numNodes++;
}

TreeNode::~TreeNode()
{
	for(TreeNode* c = _child; c; c = c->_sibling)
		delete c;
	_numNodes--;
}

TreeNode* TreeNode::child(Move move)
{
	assert(move.isValid());
	
	// See if there already is an childnode for it
	for(TreeNode* c = _child; c; c = c->_sibling)
		if(c->_move == move)
			return c;
	
	// Construct a new child node
	TreeNode* node = new TreeNode(this, move);
	node->_sibling = _child;
	_child = node;
	return node;
}

uint TreeNode::depth() const
{
	uint result = 0;
	for(TreeNode* p = _parent; p; p = p->_parent)
		++result;
	return result;
}

uint TreeNode::numVisitedChildren() const
{
	uint result = 0;
	for(TreeNode* c = _child; c; c = c->_sibling)
		++result;
	return result;
}

BoardMask TreeNode::visitedChildren() const
{
	BoardMask result;
	for(TreeNode* c = _child; c; c = c->_sibling)
		result.set(c->_move);
	return result;
}

void TreeNode::vincent(TreeNode* child)
{
	// Forget other children
	for(TreeNode* c = _child; c; c = c->_sibling)
		if(c != child)
			delete c;
	_child = child;
	_child->_sibling = nullptr;
}

std::ostream& operator<<(std::ostream& out, const TreeNode& treeNode)
{
	out << treeNode.visits() << " " << treeNode._totalScore << " " << treeNode.depth();
	for(const TreeNode* p = &treeNode; p; p = p->_parent)
		out << " " << p->_move;
	return out;
}

void TreeNode::writeOut(ostream& out, uint treshold) const
{
	if(_visits < treshold)
		return;
	out << *this << endl;
	for(TreeNode* c = _child; c; c = c->_sibling)
		c->writeOut(out, treshold);
}

void TreeNode::write(const string& filename, uint treshold) const
{
	ofstream file(filename, ofstream::out | ofstream::trunc | ofstream::binary);
	write(file, treshold);
	file.close();
}

void TreeNode::write(ostream& out, uint treshold) const
{
	assert(out.good());
	
	// Skip if the number of visits is insufficient
	if(_visits < treshold)
		return;
	
	// Count number of children that pass the threshold
	uint numTresholdChildren = 0;
	for(TreeNode* c = _child; c; c = c->_sibling)
		if(c->_visits >= treshold)
			++numTresholdChildren;
	
	// Write out this node
	out.put(_move.index());
	out.write(reinterpret_cast<const char*>(&_visits), sizeof(_visits));
	out.write(reinterpret_cast<const char*>(&_totalScore), sizeof(_totalScore));
	out.put(numTresholdChildren);
	
	// Write out child nodes
	for(TreeNode* c = _child; c; c = c->_sibling)
		if(c->_visits >= treshold)
			c->write(out, treshold);
}

void TreeNode::read(const string& filename, uint rotation)
{
	cerr << "Reading " << filename << endl;
	if(rotation == uint(-1)) {
		for(rotation = 0; rotation < Move::maxRotation; ++rotation)
			read(filename, rotation);
		return;
	}
	uint before = TreeNode::numNodes();
	ifstream file(filename, ifstream::in | ifstream::binary);
	assert(file.get() == 0xff); // Skip first node move
	read(file, rotation);
	file.close();
	cerr << "Read " << (TreeNode::numNodes() - before) << " nodes" << endl;
}

void TreeNode::read(istream& in, uint rotation)
{
	assert(in.good());
	
	// Read this node
	uint visits;
	float score;
	in.read(reinterpret_cast<char*>(&visits), sizeof(visits));
	in.read(reinterpret_cast<char*>(&score), sizeof(score));
	_visits += visits;
	_totalScore += score;
	
	// Read child nodes
	uint numChildren = in.get();
	for(uint i = 0; i < numChildren; ++i) {
		Move move = Move::fromIndex(in.get());
		assert(move.isValid());
		if(move != Move::Swap)
			move.rotate(rotation);
		TreeNode* c = child(move);
		c->read(in);
	}
}

TreeNode* TreeNode::select(const Board& board)
{
	// Index over moves
	uint visits[Move::numIndices];
	float score[Move::numIndices];
	bool valid[Move::numIndices];
	for(uint i = 0; i < Move::numIndices; ++i) {
		visits[i] = 0;
		score[i] = 0.0f;
		valid[i] = false;
	}
	
	// Swap move
	if(board.moveCount() == 1)
		valid[Move::Swap.index()] = true;
	
	// Non swap moves
	BoardMask moves = board.nonSwapMoves();
	for(BoardMask::Iterator i = moves.iterator(); i; ++i)
		valid[i->index()] = true;
	
	// Load existing child data
	for(TreeNode* c = _child; c; c = c->_sibling) {
		visits[c->_move.index()] = c->visits();
		score[c->_move.index()] = c->totalScore();
	}
	
	// Find the appropriate history heuristic
	const HistoryHeuristic& historyHeuristic = (board.player() == 1) ? HistoryHeuristic::player1 : HistoryHeuristic::player2;
	
	// Try existing children first
	uint selectedIndex = 0;
	float bestValue = 0.0;
	const float logParentVisits = log(this->visits() + 1);
	for(uint i = 0; i < Move::numIndices; ++i) {
		if(!valid[i])
			continue;
		float uctValue =
			score[i] / (visits[i] + epsilon) +
			explorationParameter * sqrt(logParentVisits / (visits[i] + epsilon)) +
			historyParameter * historyHeuristic.heuristic(i) * (1.0 / (visits[i] - score[i]  + 1.0));
		if(uctValue > bestValue) {
			selectedIndex = i;
			bestValue = uctValue;
		}
	}
	Move selectedMove = Move::fromIndex(selectedIndex);
	return child(selectedIndex + 1);
}

void TreeNode::loadGames(const string& filename)
{
	ifstream file(filename);
	if(!file.good()) {
		cerr << "Could not read: " << filename << endl;
		return;
	}
	for(string line; getline(file, line); ) {
		
		// Iterate over all symmetries
		for(uint rotation = 0; rotation < BoardPoint::maxRotation; ++rotation) {
			stringstream ss(line);
			Board board;
			TreeNode* gameState = this;
			while(ss.good()) {
				Move move;
				ss >> move;
				assert(move.isValid());
				if(move != Move::Swap)
					move.rotate(rotation);
				board.playMove(move);
				gameState = gameState->child(move);
				assert(gameState);
			}
			if(!board.gameOver()) {
				cerr << "!!! Not entire game!" << endl;
				continue;
			}
			
			/// @todo Commit score
			float value = (board.winner() == board.player()) ? 1.0 : 0.0;
			// value = 1.0 - value;
			for(uint i = 0; i < 10; ++i)
				gameState->updateStatsUpwards(value);
		}
	}
}

void TreeNode::selectAction(Board board)
{
	TreeNode* current = this;
	while(!current->isLeaf()) {
		current = current->select(board);
		board.playMove(current->_move);
	}
	if(!current->isLeaf()) {
		TreeNode* newNode = current->select(board);
		assert(newNode);
		current = newNode;
		board.playMove(current->_move);
	}
	float value = current->rollOut(board);
	current->updateStatsUpwards(value);
}

Move TreeNode::bestMove() const
{
	// Find node with highest playout count
	TreeNode* best = nullptr;
	for(TreeNode* c = _child; c; c = c->_sibling)
		if(!best || c->_visits > best->_visits)
			best = c;
	assert(best);
	return best->_move;
}

float TreeNode::rollOut(const Board& board) const
{
	// Do the bamboo bridges
	Board copy(board);
	copy.bambooBridges();
	
	// Early exit if already won
	// uint winner = copy.winner();
	// if(winner != 0)
	// 	return ((winner == board.player()) ? 1.0 : 0.0);
	
	// Do the fill-outs
	copy.randomFillUp();
	
	// 0, ½ or 1 point
	uint winner = copy.winner();
	if(winner == 1) {
		HistoryHeuristic::player1.countWin(copy.white());
		HistoryHeuristic::player2.countLoss(copy.black());
	} else if(winner == 2) {
		HistoryHeuristic::player1.countLoss(copy.white());
		HistoryHeuristic::player2.countWin(copy.black());
	}
	if(winner == 0)
		return 0.5;
	if(winner == board.player())
		return 1.0;
	else
		return 0.0;
}

void TreeNode::scaleStatistics(uint factor)
{
	_visits /= factor;
	_totalScore /= factor;
	for(TreeNode* c = _child; c; c = c->_sibling)
		c->scaleStatistics(factor);
}

void TreeNode::updateStats(float score)
{
	++_visits;
	_totalScore += score;
}

void TreeNode::updateStatsUpwards(float score)
{
	updateStats(score);
	if(_parent)
		_parent->updateStatsUpwards(1.0 - score);
}

class GameInputOutput {
public:
	GameInputOutput();
	~GameInputOutput();
	void run();
	void playMove(Move move);
	Move generateMove();
	
	TreeNode* tree() { return _tree; }
	
protected:
	Board _board;
	TreeNode* _tree;
	TreeNode* _current;
};

GameInputOutput::GameInputOutput()
: _board()
, _tree(new TreeNode())
, _current(_tree)
{
}

GameInputOutput::~GameInputOutput()
{
	delete _tree;
}

void GameInputOutput::run()
{
	cerr << "Game loop" << endl;
	string line;
	for(;;) {
		// Player move
		cin >> line;
		cerr << "In: " << line << endl;
		if(line == "Quit") {
			cerr << "Quiting" << endl;
			return;
		}
		if(line != "Start") {
			Move move(line);
			assert(move.isValid());
			playMove(move);
			if(_board.gameOver())
				break;
		}
		
		// Own move
		Move move = generateMove();
		playMove(move);
		cerr << _board << endl;
		cerr << "Out: " << move << endl;
		cout << move << endl;
		if(_board.gameOver())
			break;
	}
	cin >> line;
	cerr << "In: " << line << endl;
	cerr << "Quiting" << endl;
}

Move GameInputOutput::generateMove()
{
	cerr << "Thinking from ";
	cerr << TreeNode::numNodes()  << " nodes (" << _current->visits() << " visits)" << " (";
	cerr << (TreeNode::numNodes() * sizeof(TreeNode) / (1024*1024))  << " MB)" << endl;
	assert(_current);
	
	Timer::instance.nextRound();
	while(Timer::instance.ponder()) {
		for(uint i = 0; i < 500; ++i)
			_current->selectAction(_board);
	}
	cerr << "Thought to ";
	cerr << TreeNode::numNodes()  << " nodes (" << _current->visits() << " visits)" << " (";
	cerr << (TreeNode::numNodes() * sizeof(TreeNode) / (1024*1024))  << " MB)" << endl;
	return _current->bestMove();
}

void GameInputOutput::playMove(Move move)
{
	_board.playMove(move);
	TreeNode* vincent = _current->child(move);	
	assert(vincent);
	_current->vincent(vincent);
	_current = vincent;
	cerr << "Playing " << move << " ";
	cerr << TreeNode::numNodes()  << " nodes (" << _current->visits() << " visits)" << " (";
	cerr << (TreeNode::numNodes() * sizeof(TreeNode) / (1024*1024))  << " MB)" << endl;
}

void convertGames(const string& games, const string& out)
{
	TreeNode tree;
	tree.loadGames(games);
	tree.write(out, 20);
}

void ponder(const string& filename)
{
	Board board;
	TreeNode tree;
	tree.read(filename, -1);
	tree.scaleStatistics(10);
	for(uint i = 0; i < 1000000; ++i) {
		if(i % 100000 == 0) {
			cerr << "nodes: " << TreeNode::numNodes()  << " (" << tree.visits() << " visits)" << " (";
			cerr << (TreeNode::numNodes() * sizeof(TreeNode) / (1024*1024))  << " MB)" << endl;
		}
		tree.selectAction(board);
	}
	tree.write(filename);
}

string base64_decode(const char* encoded)
{
	static const string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int i = 0;
	int j = 0;
	int in_ = 0;
	uint8 blockIn[4];
	uint8 blockOut[3];
	std::string result;
	
	while(encoded[in_] != '=') {
		blockIn[i++] = encoded[in_];
		in_++;
		if (i ==4) {
			for (i = 0; i <4; i++)
				blockIn[i] = base64_chars.find(blockIn[i]);
			
			// Decode block
			blockOut[0] = (blockIn[0] << 2) + ((blockIn[1] & 0x30) >> 4);
			blockOut[1] = ((blockIn[1] & 0xf) << 4) + ((blockIn[2] & 0x3c) >> 2);
			blockOut[2] = ((blockIn[2] & 0x3) << 6) + blockIn[3];
			
			for (i = 0; (i < 3); i++)
				result += blockOut[i];
			i = 0;
		}
	}
	
	if (i) {
		for (j = i; j <4; j++)
			blockIn[j] = 0;
		for (j = 0; j <4; j++)
			blockIn[j] = base64_chars.find(blockIn[j]);
		
		blockOut[0] = (blockIn[0] << 2) + ((blockIn[1] & 0x30) >> 4);
		blockOut[1] = ((blockIn[1] & 0xf) << 4) + ((blockIn[2] & 0x3c) >> 2);
		blockOut[2] = ((blockIn[2] & 0x3) << 6) + blockIn[3];
		
		for (j = 0; (j < i - 1); j++)
			result += blockOut[j];
	}
	
	return result;
}

extern const char binData[];

void loadAttachement(TreeNode* tree)
{
	string attachement = base64_decode(binData);
	trace(attachement.size());
	stringstream ss(attachement);
	assert(ss.get() == 0xff); // Skip first node move
	tree->read(ss);
}

int main(int argc, char* argv[])
{
	cerr << "R " << argv[0]  << endl;
	trace(clock());
	trace(CLOCKS_PER_SEC);
	trace(sizeof(float));
	trace(sizeof(uint));
	trace(sizeof(uint64));
	trace(sizeof(void*));
	trace(sizeof(BoardPoint));
	trace(sizeof(Move));
	trace(sizeof(BoardMask));
	trace(sizeof(Board));
	trace(sizeof(TreeNode));
	trace(TreeNode::explorationParameter);
	trace(TreeNode::historyParameter);
	srand(time(0));
	
	// convertGames("competitions.txt", "games.bin");
	// convertGames("competitions.txt", "iterated.bin");
	// ponder("iterated.bin");
	
	// make && time ./polio && ls -l ./iterated.out.bin
	// bar ./iterated.out.bin2 | base64 -w 0 | ./bin2c > iterated.cpp && ls -l ./iterated.cpp
	// TreeNode tree;
	// tree.read("iterated.out.bin");
	// tree.write("iterated.out.bin2", 185); // 35
	// return 0;
	
	GameInputOutput gio;
	trace(clock());
	loadAttachement(gio.tree());
	trace(clock());
	gio.run();
	cerr << "Exit" << endl;
	return 0;
}

const char binData[] = ""; // Removed for readability, was about 1mb of base64
