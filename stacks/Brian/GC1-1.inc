<?php
/*

This file contains the logic for Game Central to take turns against an other player.
If it is executed from default.php after another player has finished their turn but only if Game Central is part of the game
It is assumed that game central is player 2

strategy3 uses the NegaMax funtions to trial all possible turn options and work out which has the most favourable outcome


/*
playmaker strategy 1
 

*/
//Global constants
define('GCREF', 1);	

fnLogMessageToDb("start GC1");

	//convert call string to upper case
	$callString = strtoupper($callString);
	
	//call function to extract the parameters from the call string
	$refs=fnFindParams($callString);

	$errCode = ""; $errMsg = ""; 
	$gameRef = 0;
	
	
	if (array_key_exists('errCode',$refs) == True) {
		$errCode = $refs['errCode'] ;
        $errMsg = $refs['errMsg'] ;
		fnLogMessageToDb($errCode.$errMsg);	}
	elseif (array_key_exists("TYPE",$refs) == False) {
		$errCode = "GC-0";
		$errDesc = "input string - type param missing"; 
		fnLogMessageToDb($errCode.$errMsg);	}
	else {
		//check callString and process accordingly
		switch ($refs["TYPE"]){
			case "GAME_STATUS":
			case "GAMESTATUS":
			case "P1_MOVE":
			case "P1_MOVES":
			case "P1MOVE":
			case "P1MOVES":
			case "P2MOVES":
			case "P2MOVE":
			case "P2_MOVE":
			case "P2_MOVES":
				if (!preg_match('/^[0-9]+$/',$refs["GAMEREF"])) {
					//game ref must be numeric
					$errCode = "GC-1";
					$errMsg = "Game Central - game ref not numeric"; 
					fnLogMessageToDb($errCode.$errMsg); }
				else {
					$gameRef = $refs["GAMEREF"]; }
		}
	}
	
	if ($gameRef == 0) {
		fnLogMessageToDb("Game Central - move message/game ref not found. Type = " . $refs["TYPE"]);}
	else {
		//game ref has been found so continue
		$gameRow = fnQrySelectGame($gameRef);
		if (is_string($gameRow)) {
			$errCode = "GC-2";
			$errMsg = "Game Central - error retrieving game record [$gameRow]";
			fnLogMessageToDb($errCode.$errMsg);}
		else {
			fnLogMessageToDb("Turn count = " . $gameRow['TurnCount']);
			//check game hasn't already finished
			if ($gameRow['Winner'] != 0) {
				$errCode = "GC-3";
				$errMsg = "Game Central - game is already finished";
				fnLogMessageToDb($errCode.$errMsg."/".$gameRow['Winner']);}
			//check GC is part of game
			elseif ($gameRow['P1Ref'] != GCREF And $gameRow['P2Ref'] != GCREF) {
				$errCode = "GC-4";
				$errMsg = "Game Central - GC is not part of game";
				fnLogMessageToDb($errCode.$errMsg."/".$gameRow['P1Ref']."/".$gameRow['P2Ref']);}
			//check it is GC's turn next
			elseif ($gameRow['LastPlayer'] == GCREF) {
				$errCode = "GC-4";
				$errMsg = "Game Central - it's not GC's turn";
				fnLogMessageToDb($errCode.$errMsg);}
		}
		
		if (strlen($errCode) == 0) {
			//set up some global constants
			define('NEXT_TO_LAST_STACK', $gameRow['AreaSize'] - 1);
			define('LAST_STACK', $gameRow['AreaSize']);
			define('P2RESERVE', $gameRow['AreaSize'] + 1);
			//no errors so get the stack records
			$stackRows = fnQrySelectStacks($gameRef,0);
			if (is_string($stackRows)) {
				$errCode = "GC-5";
				$errMsg = "Game Central - Stack not found";
				fnLogMessageToDb($errCode.$errMsg);}
			else {
				//fnStrategy1($gameRow, $stackRows); 
				fnStrategy3($gameRow);
			}
		}
	}
	
function fnStrategy3 ($gameRow) {
	global $logLevel;
	if ($logLevel>1){fnLogMessageToDb("Game Central - strategy3 - negamac");}
	$fnResp = fnNegaMax($gameRow['GameRecID'],2,0);
	if ($fnResp[0]) {
		$errCode = "GC-F";
		$errMsg = $fnResp[1];
		fnLogMessageToDb($errCode.$errMsg);
		$returnCode = 1; } 
	else {
		//print_r($fnResp);
		$fnResp = fnValidatePlay($gameRow['GameRef'],GCREF,$fnResp[1][2],$fnResp[1][1],True); 
		if ($fnResp['errCode'] != "") {
			$errCode = "GC-G";
			$errMsg = $fnResp['errCode'].$fnResp['errMsg'];
			fnLogMessageToDb($errCode.$errMsg);
			$returnCode = 1;	}
		else {
			$returnCode = 0;
			 if ($logLevel>2){fnLogMessageToDb("Game Central - successful negamax move"); } }
	}
	return $returnCode;
}
//End fnStrategy3

function fnTakeBoost($gameRow, $stackRows) {

	/*6.	Take a boost if its available
		If there's counter that can reach the far side then play it */
	$errCode = ""; $errMsg = ""; 
	//use return code to indicate what happened. -1 means no action taken, 0 means worked ok, 1 means error
	$returnCode = -1;
	
	fnLogMessageToDb("Game Central - check to see if GC can take a boost");
	if ($stackRows[2]['Top'] == GCREF And $stackRows[1]['StackHeight'] > 0) {
		fnLogMessageToDb("Top for stack 2 is " . $stackRows[2]['Top']);
		if ($stackRows[2]['StackHeight'] >= $stackRows[1]['StackHeight']) {
			fnLogMessageToDb("Game Central - can take a boost"); 
			$movePos = $gameRow['AreaSize'] -1 ;
			$fnResp = fnValidatePlay($gameRow['GameRef'],GCREF,$movePos,1,True); 
			if ($fnResp['errCode'] != "") {
				$errCode = "GC-6";
				$errMsg = "Game Central - boost - error from fnValidatePlay(" . $gameRow['GameRef'] . "," . GCREF . "," . $movePos . ",1,True)"; 
				$returnCode = 1;
				fnLogMessageToDb($errCode.$errMsg); 
				fnLogMessageToDb($errCode.$fnResp); } 
			else {
				$returnCode = 0;
				fnLogMessageToDb("Game Central - taken a boost"); }
		} 
	}
	return $returnCode;
}
//End fnTakeBoost
function fnBlockBoost($gameRow, $stackRows) {

	/*7. Block a boost if possible
		Check to see if the other player can execute a boost move next. 
		If so then use the reserve to block it. If the reserve is too small then ignore it and carry on. */
	fnLogMessageToDb("Game Central - check to see if opponent has a boost that needs blocking");

	//use return code to indicate what happened. -1 means no action taken, 0 means worked ok, 1 means error
	$returnCode = -1;
	$movePos = 0;
	$moveNum = 0;
	//if player 1 owns the stack next to the end of the playing area
	fnLogMessageToDb("stackRows[NEXT T0 LAST STACK][Top]=" . $stackRows[NEXT_TO_LAST_STACK]['Top'] );
	if ($stackRows[NEXT_TO_LAST_STACK][Top] != GCREF and $stackRows[NEXT_TO_LAST_STACK]['Top'] != 0) {
		//if P1's stack is tall enough to do the boost
		fnLogMessageToDb("stackRows[NEXT T0 LAST STACK][StackHeight]=" . $stackRows[NEXT_TO_LAST_STACK]['StackHeight'] . " stackRows[LAST STACK][StackHeight]=" . $stackRows[LAST_STACK]['StackHeight']);
		if ($stackRows[NEXT_TO_LAST_STACK]['StackHeight'] == $stackRows[LAST_STACK]['StackHeight']) {
			$movePos = 1;
			$moveNum = 1;}
		else {
			if ($stackRows[NEXT_TO_LAST_STACK]['StackHeight'] >= $stackRows[LAST_STACK]['StackHeight']) {
				//if GC has enough in reserve to block the boost
				if ($stackRows[NEXT_TO_LAST_STACK]['StackHeight'] < $stackRows[LAST_STACK]['StackHeight'] + $stackRows[P2RESERVE]['StackHeight']) {
					//block the boost by moving counters out of reserve
					$movePos = 0;
					if ($stackRows[P2RESERVE]['StackHeight'] - 1 >= $stackRows[NEXT_TO_LAST_STACK]['StackHeight'] - $stackRows[LAST_STACK]['StackHeight'] + 1) {
						$moveNum = $stackRows[P2RESERVE]['StackHeight'] - 1; }			
					else {$moveNum = $stackRows[P2RESERVE]['StackHeight']; }
				}
			}
		}
		if ($moveNum > 0) {
			fnLogMessageToDb("Game Central - move to block boost - movePos=" . $movePos . " moveNum=" . $moveNum);
			$fnResp = fnValidatePlay($gameRow['GameRef'],GCREF,$movePos,$moveNum,True); 
			if ($fnResp['errCode'] == "" or $fnResp['errCode'] == "PMO-6" or $fnResp['errCode'] == "PMO-4") {
				$returnCode = 0;
				fnLogMessageToDb("Game Central - blocked a boost"); } 
			else {
				$errCode = "GC-6";
				$errMsg = "Game Central - block - error from fnValidatePlay(" . $gameRow['GameRef'] . "," . GCREF . "," . $movePos . "," . $moveNum . ",True)"; 
				$returnCode = 1;
				fnLogMessageToDb($errCode.$errMsg); 
				fnLogMessageToDb($errCode.$fnResp); }
		}
	}
	return $returnCode;
}
//End fnBlockBoost
function fnStrategy2($gameRow, $stackRows) {

	//take a boost if possible
	$fnResp = fnTakeBoost($gameRow, $stackRows);
	if ($fnResp >= 0) {return;}
	//block opponent taking a boost if necessary and possible
	$fnResp = fnBlockBoost($gameRow, $stackRows);
	if ($fnResp >= 0) {return;}
			
	//strategy 2 is to move forward as quickly as possible which means stacks are small and furthest counter is moved first. Reserve is kept at a maximum.
	//8.	Find counter furthest from reserve that can move 
	//start from position 3 because 2 has already been checked
	fnLogMessageToDb("Game Central - look for an ordinary move further from reserve");
	for ($i = 3; $i <= LAST_STACK; $i++) {
		fnLogMessageToDb("Game Central - i= " . $i . " top = " . $stackRows[$i]['Top']) ;
		if ($stackRows[$i]['Top'] == GCREF) {
			fnLogMessageToDb("Game Central - height at i= " . $stackRows[$i]['StackHeight'] . " height at i-1= " . $stackRows[$i-1]['StackHeight']);
			if ($stackRows[$i]['StackHeight'] >= $stackRows[$i-1]['StackHeight'] ) {
				$movePos = LAST_STACK - $i + 1;
				$fnResp = fnValidatePlay($gameRow['GameRef'],GCREF,$movePos,1,True); 
				if ($fnResp['errCode'] == "" or $fnResp['errCode'] == "PMO-6" or $fnResp['errCode'] == "PMO-4") {
					fnLogMessageToDb("Game Central - moved furtherest counter - at stack ". $i); }
				else {
					$errCode = "GC-7";
					$errMsg = "Game Central - furthest - error from fnValidatePlay(" . $gameRow['GameRef'] . "," . GCREF . "," . $movePos . ",1,True)"; 
					fnLogMessageToDb($errCode.$errMsg); 
					fnLogMessageToDb($errCode.$fnResp); }
				return; 
			}
		}
	}
	//9.	If no counter in play area can move then take another one from the reserve
	fnLogMessageToDb("Game central - try moving from reserve "); 
	if ($stackRows[P2RESERVE]['StackHeight'] == 0) {
		$errCode = "GC-8";
		$errMsg = "Game Central - reserve empty - run out of move options/" . $stackRows[P2RESERVE]['StackHeight'] ; 
		fnLogMessageToDb($errCode.$errMsg); }
	else {
		$fnResp = fnValidatePlay($gameRow['GameRef'],GCREF,0,1,True); 
		if ($fnResp['errCode'] == "" or $fnResp['errCode'] == "PMO-6" or $fnResp['errCode'] == "PMO-4") {
			fnLogMessageToDb("Game Central - moved from reserve");}
		else {
			$errCode = "GC-8";
			$errMsg = "Game Central - reserve - error from fnValidatePlay(" . $gameRow['GameRef'] . "," . GCREF . ",0,1,True)"; 
			fnLogMessageToDb($errCode.$errMsg);
			fnLogMessageToDb($errCode.$fnResp);}
		return; }

}
//End fnStrategy2
function fnStrategy1($gameRow, $stackRows) {

	//take a boost if possible
	$fnResp = fnTakeBoost($gameRow, $stackRows);
	if ($fnResp >= 0) {return;}
	//block opponent taking a boost if necessary and possible
	$fnResp = fnBlockBoost($gameRow, $stackRows);
	if ($fnResp >= 0) {return;}
			
	//if opponent owns a stack then take it if possible
	fnLogMessageToDb("Game central(S1) - look to take an opponent stack");
	for ($i = LAST_STACK; $i >= 2; $i--) {
		if ($stackRows[$i]['StackHeight'] >= $stackRows[$i-1]['StackHeight'] 
		And $stackRows[$i]['Top'] == GCREF
		And $stackRows[$i-1]['Top'] != GCREF
		And $stackRows[$i-1]['Top'] != 0){
			$moveNum = 1;
			$movePos = LAST_STACK - $i + 1;
			$fnResp = fnValidatePlay($gameRow['GameRef'],GCREF,$movePos,$moveNum,True); 
			if ($fnResp['errCode'] == "" Or $fnResp['errCode'] == "PMO-6" Or $fnResp['errCode'] == "PMO-4") {
				fnLogMessageToDb("Game Central(S1) - Validate Play resp=" . $fnResp);
				fnLogMessageToDb("Game Central(S1) - took opponent stack");}
			else {
				$errCode = "GC-D";
				$errMsg = "Game Central(S1) - reserve - error from fnValidatePlay(" . $gameRow['GameRef'] . "," . GCREF . ",0," . $moveNum . ",True)"; 
				fnLogMessageToDb($errCode.$errMsg);
				fnLogMessageToDb($errCode.$fnResp);}
			return; 
		}
	}
				
	//strategy 1 is to move forward as slowly keeping stacks as tall as possible. Reserve is kept at a minimum.
	//8. If the reserve is greater than 1 then move all bar one out but make sure player 1 isn't going to boost next go
	fnLogMessageToDb("Game central(S1) - try moving from reserve");
	$availableReserve = $stackRows[P2RESERVE]['StackHeight'] - 1; 
	$lastStackHeight = $stackRows[LAST_STACK]['StackHeight'];
	$nextToLastStackHeight = $stackRows[NEXT_TO_LAST_STACK]['StackHeight'];
	$nextToLastStackOwner =  $stackRows[NEXT_TO_LAST_STACK]['Top'] ;
	if ($stackRows[P2RESERVE]['StackHeight'] > 1
		And (($availableReserve + $lastStackHeight > $nextToLastStackHeight And $nextToLastStackOwner !== GCREF) Or ($nextToLastStackOwner == GCREF))) {
		fnLogMessageToDb("Game central(S1) - moving from reserve - Avbl Rsrve=" . $availableReserve . " Lst Stk Hght=" . $lastStackHeight . " Nxt to Lst Stk Hght=" . $nextToLastStackHeight . " Nxt to Lst Stk Ownr=" . $nextToLastStackOwner);
		$moveNum = $availableReserve;
		$movePos = 0;
		$fnResp = fnValidatePlay($gameRow['GameRef'],GCREF,$movePos,$moveNum,True); 
		if ($fnResp['errCode'] == "" or $fnResp['errCode'] == "PMO-6" or $fnResp['errCode'] == "PMO-4") {
			fnLogMessageToDb("Game Central(S1) - moved from reserve");}
		else {
			$errCode = "GC-9";
			$errMsg = "Game Central(S1) - reserve - error from fnValidatePlay(" . $gameRow['GameRef'] . "," . GCREF . ",0," . $moveNum . ",True)"; 
			fnLogMessageToDb($errCode.$errMsg);
			fnLogMessageToDb($errCode.$fnResp);}
		return; }
	
	//9. Find the nearest stack that can move forward
	fnLogMessageToDb("Game Central(S1) - look for an ordinary move nearest to reserve>".LAST_STACK."<");
	for ($i = LAST_STACK; $i >= 2; $i--) {
		fnLogMessageToDb("Game Central(S1) - i=" . $i);
		//don't move from the next to last stack in case it gives ownership to other player who can then do a to boost
		if (($i == NEXT_TO_LAST_STACK And $stackRows[$i]['StackHeight'] <= $stackRows[LAST_STACK]['StackHeight']) Or ($i !== NEXT_TO_LAST_STACK)) {
			fnLogMessageToDb("Game Central(S1) - i= " . $i . " top = " . $stackRows[$i]['Top']) ;
			if ($stackRows[$i]['Top'] == GCREF) {
				fnLogMessageToDb("Game Central(S1) - height at i=" . $stackRows[$i]['StackHeight'] . " height at i-1=" . $stackRows[$i-1]['StackHeight']);
				if ($stackRows[$i]['StackHeight'] >= $stackRows[$i-1]['StackHeight'] ) {
					$movePos = LAST_STACK - $i + 1;
					$fnResp = fnValidatePlay($gameRow['GameRef'],GCREF,$movePos,1,True); 
					if ($fnResp['errCode'] == "" or $fnResp['errCode'] == "PMO-6" or $fnResp['errCode'] == "PMO-4") {
						fnLogMessageToDb("Game Central(S1) - moved nearest counter - at stack ". $i); }
					else {
						$errCode = "GC-E";
						$errMsg = "Game Central(S1) - nearest - error from fnValidatePlay(" . $gameRow['GameRef'] . "," . GCREF . "," . $movePos . ",1,True)"; 
						fnLogMessageToDb($errCode.$errMsg); 
						fnLogMessageToDb($errCode.$fnResp); }
					return; 
				}
			}
		}
	}

	//10. If no other option then take the last counter from the reserve
	fnLogMessageToDb("Game central(S1) - try moving from reserve "); 
	if ($stackRows[P2RESERVE]['StackHeight'] == 0) {
		$errCode = "GC-B";
		$errMsg = "Game Central(S1) - reserve empty - run out of move options/" . $stackRows[P2RESERVE]['StackHeight'] ; 
		fnLogMessageToDb($errCode.$errMsg); }
	else {
		$fnResp = fnValidatePlay($gameRow['GameRef'],GCREF,0,1,True); 
		if ($fnResp['errCode'] == "" or $fnResp['errCode'] == "PMO-6" or $fnResp['errCode'] == "PMO-4") {
			fnLogMessageToDb("Game Central(S1) - moved last one from reserve");}
		else {
			$errCode = "GC-C";
			$errMsg = "Game Central(S1) - reserve - error from fnValidatePlay(" . $gameRow['GameRef'] . "," . GCREF . ",0,1,True)"; 
			fnLogMessageToDb($errCode.$errMsg);
			fnLogMessageToDb($errCode.$fnResp);}
		return; }
}
//End fnStrategy1	
?>