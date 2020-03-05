
		.text
		addiu	$a0,$0,3
		addiu	$a1,$0,2
		lui	$t0,64
		addiu	$t0,$t0,8192
		sw	$a1 0($t0)	# Accessing valid memory address
		lw	$a2 0($t0)
		addiu	$a1,$a1,2
		addiu	$t0,$t0,-1
		sw	$a1 0($t0)	