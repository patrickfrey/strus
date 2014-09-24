COMPILER Strus
CHARACTERS
	letter = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_".
	digit = "0123456789".
	CR = CHR(13).
	LF = CHR(10).
	TAB = CHR(9).
	DQ = '"'.
	SQ = '\''.
	notDQ = ANY - DQ.
	notSQ = ANY - SQ.

TOKENS
	ident = ( ( letter ) { letter | digit } ).
	param = '$' digit { digit }.
	number = digit {digit}.
	double = number '.' number.
	sqstring = SQ { notSQ } SQ.
	dqstring = DQ { notDQ } DQ.
	IGNORE cr + lf + tab
	COMMENTS FROM "#" TO lf

PRODUCTIONS
	Strus = 
		Instruction
		{
		Instruction
		}.
	Instruction =
		ident
		'('
		Paramlist
		')' ';'.
	Paramlist = 
		Parameter
		{ ","
		Parameter
		}.
	Parameter =
		{
			ident
			| param
			| sqstring
			| dqstring
			| double
			| number
		}.
END Strus


