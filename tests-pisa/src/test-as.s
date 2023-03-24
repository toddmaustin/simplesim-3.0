
#
#	test-as.s: Test SimpleScalar assembler.
#
	.data
Foo:	.word		0x12345678

	.text
	.global		__start
__start:
	la $gp, _gp

	l.s $f0,Foo-4($2)
	l.s $f0,sFoo-4($2)

test_la:
	la $4, Foo
	la $4, sFoo
	la $4, 4($5)
	la $4, 4($4)
	la $4, Foo($5)
	la $4, Foo($4)
	la $4, sFoo($5)
	la $4, sFoo($4)

test_lb:
	lb $4, Foo
	lb $4, sFoo
	lb $4, ($5+$4)
	lb $4, ($5+$4)+
	lb $4, ($5+$4)-
	lb $4, ($5+$4)^+
	lb $4, ($5+$4)^-
	lb $4, 4($5)
	lb $4, 4($5)+
	lb $4, 4($5)-
	lb $4, 4($5)^+
	lb $4, 4($5)^-
	lb $4, Foo($5)
	lb $4, Foo($4)
	lb $4, Foo($5)+
	lb $4, Foo($4)+
	lb $4, Foo($5)-
	lb $4, Foo($4)-
	lb $4, Foo($5)^+
	lb $4, Foo($4)^+
	lb $4, Foo($5)^-
	lb $4, Foo($4)^-
	lb $4, sFoo($5)
	lb $4, sFoo($4)
	lb $4, sFoo($5)+
	lb $4, sFoo($4)+
	lb $4, sFoo($5)-
	lb $4, sFoo($4)-
	lb $4, sFoo($5)^+
	lb $4, sFoo($4)^+
	lb $4, sFoo($5)^-
	lb $4, sFoo($4)^-

test_sw:
	sw $4, Foo
	sw $4, sFoo
	sw $4, ($5+$4)
	sw $4, ($5+$4)+
	sw $4, ($5+$4)-
	sw $4, ($5+$4)^+
	sw $4, ($5+$4)^-
	sw $4, 4($5)
	sw $4, 4($5)+
	sw $4, 4($5)-
	sw $4, 4($5)^+
	sw $4, 4($5)^-
	sw $4, Foo($5)
	sw $4, Foo($4)
	sw $4, Foo($5)+
	sw $4, Foo($4)+
	sw $4, Foo($5)-
	sw $4, Foo($4)-
	sw $4, Foo($5)^+
	sw $4, Foo($4)^+
	sw $4, Foo($5)^-
	sw $4, Foo($4)^-
	sw $4, sFoo($5)
	sw $4, sFoo($4)
	sw $4, sFoo($5)+
	sw $4, sFoo($4)+
	sw $4, sFoo($5)-
	sw $4, sFoo($4)-
	sw $4, sFoo($5)^+
	sw $4, sFoo($4)^+
	sw $4, sFoo($5)^-
	sw $4, sFoo($4)^-

test_dlw:
	dlw $4, Foo
	add $4, $5, $4
	dlw $4, sFoo
	add $4, $5, $4
	dlw $4, ($5+$4)
	add $4, $5, $4
	dlw $4, ($5+$4)+
	add $4, $5, $4
	dlw $4, ($5+$4)-
	add $4, $5, $4
	dlw $4, ($5+$4)^+
	add $4, $5, $4
	dlw $4, ($5+$4)^-
	add $4, $5, $4
	dlw $4, 4($5)
	add $4, $5, $4
	dlw $4, 4($5)+
	add $4, $5, $4
	dlw $4, 4($5)-
	add $4, $5, $4
	dlw $4, 4($5)^+
	add $4, $5, $4
	dlw $4, 4($5)^-
	add $4, $5, $4
	dlw $4, Foo($5)
	add $4, $5, $4
	dlw $4, Foo($4)
	add $4, $5, $4
	dlw $4, Foo($5)+
	add $4, $5, $4
	dlw $4, Foo($4)+
	add $4, $5, $4
	dlw $4, Foo($5)-
	add $4, $5, $4
	dlw $4, Foo($4)-
	add $4, $5, $4
	dlw $4, Foo($5)^+
	add $4, $5, $4
	dlw $4, Foo($4)^+
	add $4, $5, $4
	dlw $4, Foo($5)^-
	add $4, $5, $4
	dlw $4, Foo($4)^-
	add $4, $5, $4
	dlw $4, sFoo($5)
	add $4, $5, $4
	dlw $4, sFoo($4)
	add $4, $5, $4
	dlw $4, sFoo($5)+
	add $4, $5, $4
	dlw $4, sFoo($4)+
	add $4, $5, $4
	dlw $4, sFoo($5)-
	add $4, $5, $4
	dlw $4, sFoo($4)-
	add $4, $5, $4
	dlw $4, sFoo($5)^+
	add $4, $5, $4
	dlw $4, sFoo($4)^+
	add $4, $5, $4
	dlw $4, sFoo($5)^-
	add $4, $5, $4
	dlw $4, sFoo($4)^-
	add $4, $5, $4

test_s.d:
	s.d $f4, Foo
	add $4, $5, $4
	s.d $f4, sFoo
	add $4, $5, $4
	s.d $f4, ($5+$4)
	add $4, $5, $4
	s.d $f4, ($5+$4)+
	add $4, $5, $4
	s.d $f4, ($5+$4)-
	add $4, $5, $4
	s.d $f4, ($5+$4)^+
	add $4, $5, $4
	s.d $f4, ($5+$4)^-
	add $4, $5, $4
	s.d $f4, 4($5)
	add $4, $5, $4
	s.d $f4, 4($5)+
	add $4, $5, $4
	s.d $f4, 4($5)-
	add $4, $5, $4
	s.d $f4, 4($5)^+
	add $4, $5, $4
	s.d $f4, 4($5)^-
	add $4, $5, $4
	s.d $f4, Foo($5)
	add $4, $5, $4
	s.d $f4, Foo($5)+
	add $4, $5, $4
	s.d $f4, Foo($5)-
	add $4, $5, $4
	s.d $f4, Foo($5)^+
	add $4, $5, $4
	s.d $f4, Foo($5)^-
	add $4, $5, $4
	s.d $f4, sFoo($5)
	add $4, $5, $4
	s.d $f4, sFoo($5)+
	add $4, $5, $4
	s.d $f4, sFoo($5)-
	add $4, $5, $4
	s.d $f4, sFoo($5)^+
	add $4, $5, $4
	s.d $f4, sFoo($5)^-
	add $4, $5, $4

	.sdata
sFoo:	.word		0x87654321
