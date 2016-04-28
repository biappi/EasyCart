<?php

if($argc != 3){
	echo 'Usage: '.$argv[0].' <in.png> <out.bin>'."\n";
	exit(1);
}

/*
** SPRITES!!!
*/

$sprite_image = imagecreatefrompng($argv[1]);

$sprites = '';
foreach(array(
	array(0*24, 0, 0x949494),
	array(1*24, 0, 0x949494),
	array(2*24, 0, 0x949494),
	array(3*24, 0, 0x949494),

	array(5*8, 0, 0xd08727),
	array(5*8, 0, 0x853921),

	array(4*24, 0, 0xd08727),
	array(4*24, 0, 0x853921),
) AS $a_sprite){
	list($x, $y, $col) = $a_sprite;
	for($y2=0; $y2<21; $y2++){
		for($x2=0; $x2<3; $x2++){
			$byte = 0;
			for($x3=0; $x3<8; $x3++){
				if((imagecolorat($sprite_image, $x+$x2*8+$x3, $y+$y2) & 0xffffff) == $col){
					$byte |= 1 << (7 - $x3);
				}
			}
			$sprites .= chr($byte);
		}
	}
	$sprites .= chr(0); // last byte, not used
}

file_put_contents($argv[2], $sprites);

?>