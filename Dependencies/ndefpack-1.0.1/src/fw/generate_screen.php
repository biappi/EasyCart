<?php

if($argc != 4){
	echo 'Usage: '.$argv[0].' <screen.bin> <cols.bin> <screen.asm>'."\n";
	exit(1);
}

#---------|---------|---------|---------
$t = <<<HERE
[% %E%a%s%y%F%l%a%s%h% ~~~~~~~~~~~~~]% %I%n%f%o% ~~~~~~~]
!                        |             |
!                        | xxxxxxxxxxx |
!                        | xxxxxxxxxxx |
!                        |             |
!                        | xxxxxxxxxxx |
!                        |             |
!                        |             |
!                        |             |
!                        |             |
!                        |             |
!                        |             |
!                        |             |
!                        |             |
!                        |             |
!                        |             |
!                        |             |
!                        |             |
!                        |             |
!                        |             |
!                        |             |
!                        |             |
!                        | xxxxxxxxxx  |
!                        | xxxxxxxx    |
{------------------------}-------------}
HERE;
#---------|---------|---------|---------
$help = <<<HERE
 To navigate
 use key/joy
 up or down 
 or<%F%5=%F%7>

 Scroll page
 left/right
 or<%F%6=%F%8>

 Start Prog:
<%F%1=%R%E%T=%F%I%R%E>

 Quit:<%F%2>
HERE;
#---------|---------|---------|---------
$msg = <<<HERE
[~~~~~~~~~~~~~]
!             |
! Please Wait |
!             |
{-------------}
HERE;
#---------|---------|---------|---------


/*
** ENCODE SCREEN
*/

// mapping
$chars = array(
  '[' => chr(0),
  '~' => chr(1),
  ']' => chr(2),

  '!' => chr(128),
  '{' => chr(129),
  '-' => chr(130),
  '}' => chr(131),
  '|' => chr(132),

  '<' => chr(158),
  '>' => chr(30),
  '=' => chr(224),
);

// apply mapping, inverse chars, split
$t = strtr($t, $chars);
$t = preg_replace('!%(.)!e', 'chr(0x80 + ord("\1"))', $t);
$t = explode("\n", $t);

// apply mapping, inverse chars, split
$help = strtr($help, $chars);
$help = preg_replace('!%(.)!e', 'chr(0x80 + ord("\1"))', $help);
$help = explode("\n", $help);

// apply mapping, inverse chars, split
$msg = strtr($msg, $chars);
$msg = preg_replace('!%(.)!e', 'chr(0x80 + ord("\1"))', $msg);
$msg = explode("\n", $msg);

// paste help to screen
foreach($help AS $k=>$v){
	$kk = $k + 7;
	$t[$kk] = substr($t[$kk], 0, 26).str_pad($v, 13, ' ').substr($t[$kk], -1);
}

// paste msg to screen
foreach($msg AS $k=>$v){
	$kk = $k + 10;
	$t[$kk] = substr($t[$kk], 0, 5).str_pad($v, 15, ' ').substr($t[$kk], 5+15);
}

// apply custom logl chars (x's in the screen above)
foreach(array(
	 2*40+27 => array_merge(range(0x14, 0x18),array(0x20),range(0x19,0x1d)),
	 3*40+27 => array_merge(range(0x94, 0x98),array(0x20),range(0x99,0x9d)),
	 5*40+27 => range(0x85, 0x8f),
	22*40+27 => array_merge(range(0x90, 0x93),range(0x06,0x0b)),
	23*40+27 => range(0x0c, 0x13),
) AS $o1 => $data){
	foreach($data AS $o2 => $val){
		$t[floor($o1/40)][$o1 % 40 + $o2] = chr($val);
	}
}

$screen = repair_case(implode('', $t));

// setup sprite pointers (in screen ram)
$screen .= str_repeat(chr(0), 16).chr(0x20).chr(0x21).chr(0x22).chr(0x23).chr(0x24).chr(0x25).chr(0x26).chr(0x27);

file_put_contents($argv[1], $screen);

/*
** COLORS!
*/


$col_main_border = 5; //GREEN
$col_main_inner = 13; //LIGHT_GREEN
$col_main_wait = 3; //CYAN
$col_info_border = 6; //BLUE
$col_info_inner = 14; //LIGHT_BLUE
$col_slider = 3;

// some different colored lines

$LINE['col_line_top'] =
	str_repeat(chr($col_main_border), 26).
	str_repeat(chr($col_info_border), 14);
$LINE['col_line_head'] =
	str_repeat(chr($col_main_border), 1).
	str_repeat(chr($col_main_inner), 24).
	str_repeat(chr($col_main_border), 1).
	str_repeat(chr(1), 13).
	str_repeat(chr($col_info_border), 1);
$LINE['col_line_help'] =
	str_repeat(chr($col_main_border), 1).
	str_repeat(chr($col_main_inner), 24).
	str_repeat(chr($col_main_border), 1).
	str_repeat(chr($col_info_inner), 13).
	str_repeat(chr($col_info_border), 1);
$LINE['col_line_box_a'] =
	str_repeat(chr($col_main_border), 26).
	str_repeat(chr($col_info_inner), 13).
	str_repeat(chr($col_info_border), 1);
$LINE['col_line_box_b'] =
	str_repeat(chr($col_main_border), 6).
	str_repeat(chr($col_main_wait), 13).
	str_repeat(chr($col_main_border), 7).
	str_repeat(chr($col_info_inner), 13).
	str_repeat(chr($col_info_border), 1);

// which pattern should be at what line

$col_pattern = array(
	'col_line_top',
	'col_line_help',
	'col_line_head',
	'col_line_head',
	'col_line_help',
	'col_line_help',
	'col_line_help',
	'col_line_help',
	'col_line_help',
	'col_line_help',
	'col_line_box_a',
	'col_line_box_a',
	'col_line_box_b',
	'col_line_box_a',
	'col_line_box_a',
	'col_line_help',
	'col_line_help',
	'col_line_help',
	'col_line_help',
	'col_line_help',
	'col_line_help',
	'col_line_help',
	'col_line_head',
	'col_line_help',
	'col_line_top',
);

// encode (end = $0400)
$end = 0x400;
// lines
foreach($LINE AS $na => $dummy){
	$end -= 40;
	$ofs[$na] = $end;
}
// pattern
$end -= 25;
$ofs['col_pattern'] = $end;
$ofs['start_cols_screen_sprites'] = $end;

/*
** ENCODE COLORS
*/

$cols = '';
foreach($col_pattern AS $ln){
	$cols .= chr($ofs[$ln] & 0xff); // just lower byte
}
asort($ofs);
foreach($ofs AS $na => $dummy){
	if(isset($LINE[$na])){
		$cols .= $LINE[$na];
	}
}

// output
file_put_contents($argv[2], $cols);

/*
** output offsets
*/
$f = fopen($argv[3], 'w');
foreach($ofs AS $na => $pos){
	fwrite($f, $na.' = '.sprintf('$%x', $pos)."\n");
}
fwrite($f, 'color_slider_off = '.$col_main_border."\n");
fwrite($f, 'color_slider_on = '.$col_slider."\n");
fclose($f);

/*
** change case: ascii -> petscii (incl. inverse chars)
*/

function repair_case($t){
	for($i=0; $i<strlen($t); $i++){
		$o = ord($t[$i]);
		if($o >= 0x41 && $o <= 0x5a){
			$t[$i] = chr($o + 0x20);
		}else if($o >= 0x61 && $o <= 0x7a){
			$t[$i] = chr($o - 0x20);
		}
		if($o >= 0xc1 && $o <= 0xda){
			$t[$i] = chr($o + 0x20);
		}else if($o >= 0xe1 && $o <= 0xfa){
			$t[$i] = chr($o - 0x20);
		}
	}
	return $t;
}

?>