<?php

if($argc != 3){
	echo 'Usage: '.$argv[0].' <in.png> <out.bin>'."\n";
	exit(1);
}

/*
** FONT
*/

$img = imagecreatefrompng($argv[1]);
$font = '';
for($y=0; $y<8; $y++){
	for($x=0; $x<32; $x++){
		for($cy=0; $cy<8; $cy++){
			$c = 0;
			for($cx=0; $cx<8; $cx++){
				if((imagecolorat($img, $x*8+$cx, $y*8+$cy) & 0xffffff) == 0x000000){
					$c |= 1 << (7 - $cx);
				}
			}
			$font .= chr($c);
		}
	}
}

file_put_contents($argv[2], $font);

?>