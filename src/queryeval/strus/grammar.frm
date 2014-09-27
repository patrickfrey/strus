COMPILER Strus
CHARACTERS
	letter = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_".
	digit = "0123456789".
	CR = "\r".
	LF = "\n".
	TAB = "\t".
	DQ = '"'.
	SQ = '\''.
	notDQ = ANY - DQ.
	notSQ = ANY - SQ.

TOKENS
	ident = ( ( letter ) { letter | digit } ).
	param = '$' digit { digit }.
	number = digit {digit}.
	fpnumber = digit {digit} '.' digit {digit}.
	sqstring = SQ { notSQ } SQ.
	dqstring = DQ { notDQ } DQ.
	COMMENTS FROM "--" TO "\n"
	IGNORE CR + LF + TAB

PRODUCTIONS
	Parameter =
	{
		ident
		| param
		| sqstring
		| dqstring
		| fpnumber
		| number
	}.
	Instruction =
		ident
		'('
		Parameter
		{ ","
		Parameter
		}
		')' ';'.
	Strus = 
		Instruction
		{
		Instruction
		}.
END Strus.


