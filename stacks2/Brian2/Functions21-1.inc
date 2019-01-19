<?php
	
/* Stacks version 1-2. 
Players can now move more than one counter at a time in the playing area.
In version 1-1 a player can only ever move 1 counter at a time so long the current stack was of equal or greater height as the next. Now up to n counters can be moved together so long as the current stack is n-1 counters taller than the next. (When the current and next stacks are of equal height then only 1 counter can be moved.) The ownership of the counters that are moved doesn't change so a player can move his opponent's counters.
All other rules are the same as before.
*/
/* Stacks version 1-3. 
Introduce transactions so that database updates are done consistently i.e. they all work or all are rolled back.
Read only queries (SELECTs) are ok becuase they return a value which is checked for errors.
Update/Add queries don't return any data so they will need to return a value to show if they worked or not
	fnQryAddPlayer [Done]
	fnQryUpdateRefs [No change required]
	fnQryAddStack [Used in fnProcessMove (Done) and fnSetUpGameRecs/fnProcessNewP1 (Done)]
	fnQryUpdateStack [Used in fnProcessMove (Done) and fnSetUpPlayer2/fnProcessNewP2 (Done)]
	fnQryAddGame [Used in fnProcessMove (Done) and fnSetUpGameRecs/fnProcessNewP1 (Done)]
	fnQryUpdateGame [Used in fnProcessMove (Done) and fnProcessInput/RESIGN (Done)]
	fnQryAddCounter  [Used in fnProcessMove (Done), fnSetUpGameRecs/fnProcessNewP2 (Done) and fnSetUpPlayer2/fnProcessNewP2 (Done)]
	fnQryUpdateCounter [Used in fnProcessMove (done)]
	(LogMessage)
	(TidyLog)
*/
/* Stacks version 1-4.
In this version the computer - Game Central - will use minimax or negamax (nmx for short) artificial intelligence routines to decide its moves.

************
Things to do
************
	
	New function called fnProcessNmxMove
		Determine if GameKey is a GameRecID or a Nmx hash key - check length of the key is less than 20 [Done]
		Calculate a new hash key [Done]
		Use fnValidatePlay if the GameKey is GameRecID otherwise use a new function that does the validation for Nmx games [Done]
		Create a new function for validating Nmx games [done]
		Create a new function to make a Nmx move - fnMakeNmxMove [done]
	New function called fnEvaluateGame to calculate a score for a given negamax game. Score from the perspective of the last player. [done]
	New sql functions required to 
		select nmx_counters [Done]
		select nmx_stacks [done]
		select nmx_games [Done]
		select game record using GameRecID [Done]
		Update nmx_counters, nmx-stakcs, nms-games [done]
		Store new nmx_counters, nmx-stakcs, nms-games [done]
	Need to store logging messages and (re)write them to the database AFTER the rollback in fnProcessMove [Done]
	Need to store logging messages and (re)write them to the database AFTER the rollback in fnProcessInput [Done]
	Add an extra exception to the delete and update SQL queries to make sure that the expected number of records have been deleted or updated. See delete games for example [Done] 
*/
/* Stacks version 1-5.
Combined the negamax functions into a routine to work out the best next move
*/
/* Stacks version 1-6.
New rule allowing counters moved out of reserve to be advance more than one step. The aim is to get the game started more quickly.
To do:
	fnProcessMove - for moves out the reserve allow direct to any step that is less than AreaSize/2 where the steps up to and including the Target Step are empty
		Recognise that MovePos being a decimal between 0.1 and 0.9 is a move out the reserve [done]
		Recognise that if MovePos is a decimal then the part after the decimal point is the Target Step [done]
		
		Check that steps up to and including Target Step are empty otherwise generate an error

	fnMakeNmxMove - as above [done]

	fnProcessInput - allow input for a Move Pos to denote a move from the reserve straight to step 1, 2, 3 ... or 9
		valid value for Move Pos are any +ve integer as before plus 0.1 to 0.9 [Done]

	fnValidatePlay
		Recognise that MovePos being a decimal between 0.1 and 0.9 is a move out the reserve [done]
		Recognise that if MovePos is a decimal then the part after the decimal point is the Target Step [done]
		Check that if MovePos is a decimal then the number is zero and the decimal part is between 1 and 9 [done]
		Check that Target Step is less than AreaSize/2 otherwise generate an error [Done]
		Check that if Target Step is greater than 1 then all steps must be empty [Done]

	fnValidateNmxPlay - as fnValidatePlay [Done]

	fnGetMoves - for moves out the reserve randomly choose from valid value for Move Pos [done]

*/
/* Stacks version 2-0 (21-0).
Stacks version 2 has a differnt playing area to the original Stacks. 
The playing are is a 2 digit number e.g. 24 which means the playing area is 2 rows or Tracks by 4 steps.
The code is stored in folder stacks2 and stacks2/Brian2
The database structure is also different in thsi version. The Counters table has been removed and the data merged into the stacks table.
*/
/* Stacks version 2-1 (21-1).
Removed the negamax functions.
Update fnGetMoves to collect all possible moves for v2 game.
Create fnFindNextStack
Create fnEncodeBoard to change a set of stacks into a string of data 
Update fnProcessMove to account for new move rules and changes to stack record and removal of counters record
Update fnCheckLoser
The index for stackRows is now the stackNum e.g. 800,801,802, ...etc (instead of 0,1,2,.. etc)

*/

//Global variables for database queries
$conn; 
$qryAddPlayer; $qrySelectPlayer; 
$qryUpdateRefs; $qrySelectRefs; 
$qryAddStack; $qrySelectStacks; $qryUpdateStack; $qryDeleteStacksForManyTurns; $qryDeleteStacksForOneTurn;
$qryAddGame; $qrySelectGame; $qrySelectGameByID; $qryUpdateGame; $qrySelectGamesForPlayer; $qryDeleteGames;
$qryAddCounter; $qrySelectCounters; $qryUpdateCounter; $qrySelectAllCounters; $qryDeleteCountersForManyTurns; $qryDeleteCountersForOneTurn;
$qrySelectOpenGames; 
$qryAddNmxCounter; $qrySelectNmxCounters; $qrySelectAllNmxCounters; $qryUpdateNmxCounter; $qryAddNmxStack; $qrySelectNmxStacks; $qryUpdateNmxStack;
$qryAddNmxGame; $qrySelectNmxGame; $qryUpdateNmxGame;
$qryLogMessage; $qryTidyLog;

//Global variables
$tbP1NewPlyrName; $tbP1NewGameRef; $tbP1OldGameRef;
$tbP2NewPlyrName; $tbP2NewGameRef; $tbP2OldGameRef;

//for message logging
$callString;
require	"Functions23.inc";

function fnEvaluateGame ($gameKey, $gameRow) {
	
	fnLogMessageToDb("start fnEvaluateGame GK=" . $gameKey) ;
	/*calcualte the score from the perspective of the next player
	pScore: Position of stacks owned relative to opponent. Score for each stack = Height/((counters)/100)*(stacknum/stackcount)
	rScore: Size of reserve relative to opponent
	sScore: Proportion of stacks owned
	*/
	global $logLevel, $pScoreFactor, $rScoreFactor, $sScoreFactor;
	$errCode="";
	//find out if gameKey is GameRecID or Nmx hash key
	if (strlen($gameKey)<20) {
		$gameRecID=$gameKey; }
	else {
		$gameRecID = 0;
		$hashKey = $gameKey; }

	//get the relevant game and stacks records
	if ($gameRecID>0) {
		//get stacks records
		if ($logLevel>2){ fnLogMessageToDb("(fnEvaluateGame) get stacks records using GameRef and TurnCount"); }
		$stackRows = fnQrySelectStacks($gameRow['GameRecID'], $gameRow['TurnCount']);
		if (is_string($stackRows)) {
			$errCode = "EVG-4";
			$errMsg = $stackRows; 
			if ($logLevel>1){ fnLogMessageToDb ($errCode.$errMsg); } }
	}
	else {
		//get stacks records
		if ($logLevel>2){ fnLogMessageToDb("(fnEvaluateGame) get stacks records using hashKey"); }
		$stackRows = fnQrySelectNmxStacks($hashKey);
		if (is_string($stackRows)) {
			$errCode = "EVG-2";
			$errMsg = $stackRows;
			if ($logLevel>1){ fnLogMessageToDb ($errCode.$errMsg); } }
	}
	//check for errors 
	if (strlen($errCode)==0) {
		//Calculate stack position score
		$pScore = 0; $p1StackCount = 0; $p2StackCount = 0;
		$maxI = count($stackRows) - 1;
		//loop through the stacks missing out the first (player 1 reserve) and the last (player 2 reserve)
		for ($i=1; $i<$maxI; $i++) {
			//ignore empty stacks
			if ($stackRows[$i]['OwnerPNum']>0) {
				//different calcualations for P1 and P2
				if ($stackRows[$i]['OwnerPNum'] == $gameRow['P1RecID']) {
					$p1StackCount += 1;
					//Score for each stack = Height/((counters)/100)*(stack number/(stackcount-1))
					$stackScore = $stackRows[$i]['StackHeight']/((($gameRow['AreaSize']+2)*2)/100)*($i/($gameRow['AreaSize']-1)); }
				else {
					$p2StackCount += 1;
					//Score for each stack = Height/((counters)/100)*(stack number/(stackcount-1))
					$stackScore = $stackRows[$i]['StackHeight']/((($gameRow['AreaSize']+2)*2)/100)*(($gameRow['AreaSize']-$i+1)/($gameRow['AreaSize']-1)); }
				//the score for stacks belonging to the next player are added to the overall stack score.The opponent's score are deducted
				if ($stackRows[$i]['OwnerPNum'] == $gameRow['LastPlayerID']) {
					$pScore -= $stackScore; }
				else {
					$pScore += $stackScore; }
			}
		}
		//Calculate stack score = (stacks owned by player -  stacks owned by opponent) / (stacks owned by player + stacks owned by opponent) * 100
		if ($gameRow['P2RecID'] == $gameRow['LastPlayerID']) {
			$sScore = ($p1StackCount - $p2StackCount) / ($p1StackCount + $p2StackCount) * 100; }
		else {
			$sScore = ($p2StackCount - $p1StackCount) / ($p1StackCount + $p2StackCount) * 100; }
		
		//Calculate reserve score = (size of player's reserve - size of opponent's reserve) / number of counters * 100 
		if ($gameRow['P2RecID'] == $gameRow['LastPlayerID']) {
			$rScore = ($stackRows[0]['StackHeight'] - $stackRows[$gameRow['AreaSize']+1]['StackHeight']) / (($gameRow['AreaSize']+2)*2) * 100; }
		else {
			$rScore = ($stackRows[$gameRow['AreaSize']+1]['StackHeight'] - $stackRows[0]['StackHeight']) / (($gameRow['AreaSize']+2)*2) * 100; }
		
		//apply weighting factors and calculate score
		$pScore = $pScore * $pScoreFactor;
		$rScore = $rScore * $rScoreFactor;
		$sScore = $sScore * $sScoreFactor;
		$totalScore = $pScore + $rScore + $sScore;
		
		//update nmx game record with score
		//only update if nmx game ($gameRecID = 0) and the score has changed
		fnLogMessageToDb("GR $gameRecID , HK " . $gameRow['HashKey'] . ", totalScore $totalScore , previous score " . $gameRow['GameScore']);
		if ($gameRecID == 0 And round($totalScore,2) != $gameRow['GameScore']) {
			$fnResp = fnQryUpdateNmxGame($gameRow['HashKey'],$gameRow['P1RecID'],$gameRow['P2RecID'],$gameRow['LastPlayerID'],$gameRow['AreaSize'],$gameRow['Winner'],$gameRow['TurnCount'],$totalScore);
			if (strlen($fnResp) > 0) {
				$errCode = "EVG-5";
				$errMsg = $fnResp; 
				fnLogMessageToDb($errCode.$errMsg); 
				return array(1,$errCode.$errMsg); }
		}
		
		//return score
		if ($logLevel>1){ fnLogMessageToDb("fnEvaluateGame ends successfully and returns scores. pS=" . $pScore . " rS=" . $rScore . " sS=" . $sScore);}
		return array(0,round($totalScore,2));
	}
	else {
		//errors retrieving records so return error details
		if ($logLevel>2){ fnLogMessageToDb("fnEvaluateGame ends unsuccessfully and returns error");}
		return array(1,$errCode.$errMsg) ; }

}
// End fnEvaluate
function fnValidatePlayerNamePW($playerNamePW) {
	fnLogMessageToDb("Start fnValidatePlayerNamePW"); 
	global $logLevel;

	//separate player name and PIN
	//search for slash
	$inx = strpos($playerNamePW,"/",0); 
	if ($inx == FALSE){
		$errCode = 'VP-1';
		$errMsg = 'Player name and\or PIN is incorrect';
		fnLogMessageToDb($errCode.$errMsg);
		return $errCode.$errMsg; }
	$playerName = substr($playerNamePW,0,$inx);
	$password = substr($playerNamePW,$inx+1,strlen($playerNamePW)-$inx);
	//search for player name
	$playerRow =fnQrySelectPlayer($playerName);
	if (is_string($playerRow)) {
		$errCode = 'VP-2';
		$errMsg = 'Player name and\or PIN is incorrect';
		fnLogMessageToDb($errCode.$errMsg);
		return $errCode.$errMsg; }
	elseif ($password != $playerRow['Password']){
		$errCode = 'VP-3';
		$errMsg = 'Player name and\or PIN is incorrect';
		fnLogMessageToDb($errCode.$errMsg);
		return $errCode.$errMsg; }
	//all good so return player ref
	return $playerRow['PlayerRef'];

}
//End fnValidatePlayerNamePW
function fnValidatePlay($tbPOldGameRef,$tbPOldPlyrRef,$inputMovePos,$moveNum,$procMove = "Y") {
	fnLogMessageToDb("start fnValidatePlay");
	$errMsg = ""; $errCode = "";
	global $logLevel;
	
	//get game record
	if (strlen($tbPOldGameRef)>0) {
		$gameRef = $tbPOldGameRef;}
	else {
		$errMsg = "Game ref not found in subValidatePlay. Fatal error.";
		$errCode = "VPY-1";
		fnLogMessageToDb($errCode.$errMsg);
		return array("errCode"=>$errCode,"errMsg"=>$errMsg); }

	$gameRow = fnQrySelectGame($gameRef); 
	if (is_string($gameRow)) {
		$errMsg = "Game " . $gameRef . " not found";
		$errCode = "VPY-2";
		fnLogMessageToDb($errCode.$errMsg);
		return array("errCode"=>$errCode,"errMsg"=>$errMsg); }

	if ($gameRow['Winner'] != 0){
		$errMsg = "Game " . $gameRef . " has already been won";
		$errCode = "VPY-3";
		fnLogMessageToDb($errCode.$errMsg);
		return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
		
	if ($tbPOldPlyrRef == $gameRow['P1RecID']) {
		$player1or2 = 1;}
	elseif ($tbPOldPlyrRef == $gameRow['P2RecID']) {
		$player1or2 = 2;}
	else {
		$errMsg = "Incorrect player ".$tbPOldPlyrRef;
		$errCode = "VPY-4";
		fnLogMessageToDb($errCode.$errMsg);
		return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
		
	//1. Check it is the right player taking a turn.
	if ($player1or2 == 1) {
		if ($gameRow['LastPlayerID'] == $gameRow['P1RecID']) {
			$errMsg = "Not player 1's turn";
			$errCode = "VPY-H";
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
	}
	else {
		if ($gameRow['LastPlayerID'] == $gameRow['P2RecID']) {
			$errMsg = "Not player 2's turn";
			$errCode = "VPY-5";
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
	}

	//if movePos is a decimal then it must be between 0.1 and 0.9
	if (strpos($inputMovePos,'.')===False) {
		//not a decimal so do nothing
		$movePos = $inputMovePos; $movePos2 = 0;
	}
	else {
		list($movePos, $movePos2) = explode('.', $inputMovePos);
		//movePos is a decimal so decimal part must be it must be between 1 and 9
		if ($movePos2<1 or $movePos2>9) {
			$errMsg = "Player " . $player1or2 . ": Invalid move position. MN= {$moveNum} MP= {$movePos}.{$movePos2}";
			$errCode = 'VPY-I';
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
		elseif ($movePos > 0) {
			//movePos is a decimal so number part must be zero
			$errMsg = "Player " . $player1or2 . ": Invalid move position. MN= {$moveNum} MP= {$movePos}.{$movePos2}";
			$errCode = 'VPY-L';
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
	}
		
	//move position must not be blank or greater than areaSize
	if ($movePos > $gameRow['AreaSize']) {
		$errMsg = "Player " . $player1or2 . ": Invalid move position. MN= {$moveNum} MP= {$movePos}.{$movePos2}";
		$errCode = "VPY-7";
		fnLogMessageToDb($errCode.$errMsg);
		return array("errCode"=>$errCode,"errMsg"=>$errMsg); }

	//move num can be blank unless the move position is 0
	if ($moveNum == 0) {
		if ($movePos == 0) {
			$errMsg = "Player " . $player1or2 . ": Invalid move num = 0";
			$errCode = "VPY-8";
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
		else {
			//not moving reserve so assume move num is 1
			$moveNum = 1; }
	}
	//get stack records
	$stackRows = fnQrySelectStacks($gameRef,0);
	if (is_string($stackRows)) {
		$errMsg = "Stack not found in sub ValidatePlay. Fatal error.(P" . $player1or2 . ")";
		$errCode = "VPY-A";
		fnLogMessageToDb($errCode.$errMsg);
		return array("errCode"=>$errCode,"errMsg"=>$errMsg); }

	//the move num must be less than or equal to the stack height
	if ($player1or2 == 1) {
   	if ($moveNum > $stackRows[$movePos]['StackHeight']) {
			$errMsg = "Player 1: Invalid move count. MN= {$moveNum} MP= {$movePos}.{$movePos2}";
			$errCode = "VPY-B";
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
	}
	else {
		if ($moveNum > $stackRows[$gameRow['AreaSize'] + 1 - $movePos]['StackHeight']){
			$errMsg = "Player 2: Invalid move count. MN= {$moveNum} MP= {$movePos}.{$movePos2}";
			$errCode = "VPY-C";
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
	}
	if ($movePos != 0) {
		//the move position is not for the reserve so ...
		if ($player1or2 == 1) {
			//... the top counter must belong to player 1
			if ($stackRows[$movePos]['OwnerPNum'] <> $gameRow['P1RecID']) {
				$errMsg = "Player 2: Invalid move position. MN= {$moveNum} MP= {$movePos}.{$movePos2}";
				$errCode = "VPY-D";
				fnLogMessageToDb($errCode.$errMsg);
				return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
			//... and the next stack must not be higher than the current one less the move num plus 1 (this limits how many counters can be moved)
			elseif ($stackRows[$movePos+1]['StackHeight'] > $stackRows[$movePos]['StackHeight'] - $moveNum + 1 ) {
				$errMsg = "Player 1: Invalid move position. MN= {$moveNum} MP= {$movePos}.{$movePos2}";
				$errCode = "VPY-E";
				fnLogMessageToDb($errCode.$errMsg);
				return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
		}
		else {
			//... the top counter must belong to player 2
			if ($stackRows[$gameRow['AreaSize']+1 - $movePos]['OwnerPNum'] <> $gameRow['P2RecID']) {
				$errMsg = "Player 2: Invalid move position. MN= {$moveNum} MP= {$movePos}.{$movePos2}";
				$errCode = "VPY-F";
				fnLogMessageToDb($errCode.$errMsg);
				return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
			//... and the next stack must not be higher than the current one less the move num plus 1 (this limits how many counters can be moved) 
			elseif ($stackRows[$gameRow['AreaSize']+1 - $movePos -1]['StackHeight'] > $stackRows[$gameRow['AreaSize']+1 - $movePos]['StackHeight'] - $moveNum + 1) {
				$errMsg = "Player 2: Invalid move position. MN= {$moveNum} MP= {$movePos}.{$movePos2}";
				$errCode = "VPY-G";
				fnLogMessageToDb($errCode.$errMsg);
				return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
		}
	} else {
		//move position is from the reserve so ...
		if ($movePos2>0) {
			//movePos is a decimal indicating that the counters are to be moved move one or more steps into the playing area
			//movePos2 must be less than half the AreaSize
			if ($movePos2 >= $gameRow['AreaSize']/2) {
				$errMsg = "Player $player1or2: Invalid move position. MN= {$moveNum} MP= {$movePos}.{$movePos2}";
				$errCode = "VPY-J";
				fnLogMessageToDb($errCode.$errMsg);
				return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
			//if moving more than one step then all the steps up to an including the movePos2 must be empty
			if ($movePos2 >1 ) {
				for ($i=2; $i<=$movePos2; $i++){
					if ($player1or2 == 1){
						if($stackRows[$i]['StackHeight']>0) {
							$errMsg = "Player $player1or2: Invalid move position. MN= {$moveNum} MP= {$movePos}.{$movePos2}";
							$errCode = "VPY-M";
							fnLogMessageToDb($errCode.$errMsg);
							return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
					}
					if ($player1or2 == 2){
						if($stackRows[$gameRow['AreaSize'] + 1 - $i]['StackHeight']>0) {
							$errMsg = "Player $player1or2: Invalid move position. MN= {$moveNum} MP= {$movePos}.{$movePos2}";
							$errCode = "VPY-K";
							fnLogMessageToDb($errCode.$errMsg);
							return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
					}
				}
			}
		}
	}
	
	if ($procMove == "Y") {
		$fnResp = fnProcessMove($inputMovePos, $moveNum, $gameRef, True);
		$errCode = $fnResp["errCode"];
		$errMsg = $fnResp["errMsg"]; }

	return array("errCode"=>$errCode,"errMsg"=>$errMsg); 
}
//End fnValidatePlay
function fnProcessMove($gameRecID, $moveDir, $movePos, $silent=0) {
	//Move a player's counters 
	// movePos is stack where move starts, moveDir is direction of move where 1 = left, 2 = forward, 3 = right

	global $conn, $logLevel;
	fnLogMessageToDb('start fnProcessMove. GID='.$gameRecID.' MD='.$moveDir.' MP='.$movePos);
	
	$boostMove = False;
	$errMsg = ""; $errCode = "";
	$errCode2 = ""; $errMsg2 = "";

	//begin database transaction
	$conn->beginTransaction();
	$logMsgs=array("rewrite log msgs after rollback");

	$gameRow = fnQrySelectGameByID($gameRecID);	
	if (is_string($gameRow)) {
		$errCode = "PMO-1";
		$errMsg = $gameRow; 
		fnLogMessageToDb($errCode.$errMsg);
		$logMsgs[] = $errCode.$errMsg; }
	else {
		//get stacks
		$stackRows = fnQrySelectStacks($gameRow['GameRef'],0);
		if (is_string($stackRows)) {
			$errMsg = $stackRows;
			$errCode = "PMO-2";
			fnLogMessageToDb($errCode.$errMsg);
			$logMsgs[] = $errCode.$errMsg;	}
		else{
			fnLogMessageToDb('call to fnQrySelectStacks worked. GameRef='.$gameRow['GameRef']);
			$logMsgs[] = 'call to fnQrySelectStacks worked. GameRef='.$gameRow['GameRef'];
		}
	}
	//only continue if there are no errors
	if (strlen($errCode) == 0) {
		//rules for moves are the same for the whole board. 
		// movePos is stack where move starts, moveDir is direction of move where 1 = left, 2 = forward, 3 = right
		// need to know which player is moving in order to interpret move direction correctly

		//find index of current stack record
		$stackRowsIndex = $movePos;
		/*
		$numStackRows = count($stackRows);
		$stackRowsIndex = 0;
		$notFound = True;
		while ($notFound) {
			if ($stackRows[$stackRowsIndex]['StackNum'] == $movePos) {
				$notFound = False; 
				//check stack belongs to next player
				if ($gameRow['LastPlayerID'] == $gameRow['P1RecID'] and $stackRows[$stackRowsIndex]['OwnerPNum'] != 2) {
					$errCode = "PMO-3A";
					$errMsg = "stack " . $movePos . " belongs to " . $stackRows[$stackRowsIndex]['OwnerPNum'] . " but next player is P2";
				}
				elseif ($gameRow['LastPlayerID'] == $gameRow['P2RecID'] and $stackRows[$stackRowsIndex]['OwnerPNum'] != 1) {
					$errCode = "PMO-3A";
					$errMsg = "stack " . $movePos . " belongs to " . $stackRows[$stackRowsIndex]['OwnerPNum'] . " but next player is P1";
				} 
			}
			else {
				$stackRowsIndex++; 
				if ($stackRowsIndex == $numStackRows) {
					$notFound = False;
					$errCode = "PMO-3";
					$errMsg = "Stack not found for stack num " . $movePos;
					fnLogMessageToDb($errCode.$errMsg);
					$logMsgs[] = $errCode.$errMsg;
				}
			}
		} */
	}
	
	//only continue if there are no errors
	if (strlen($errCode) == 0) {
		//calculate track and stack numbers
		//step num is third and any subsequent digits of the stack num
		$currStepNum = intval(substr($movePos,2,strlen($movePos)-2));
		echo "<br>"." CS:".$currStepNum;
		//track is second digit of the stack num
		$currTrackNum = intval(substr($movePos,1,1));
		echo ' CT:'.$currTrackNum.' MD'.$moveDir.'<br>';
		
		//calculate stackNum of next stack
		if ($gameRow['LastPlayerID'] == $gameRow['P1RecID']) {
			//player 2's move
			if ($moveDir == 3) {
				//increment trackNum
				$nextTrackNum = $currTrackNum + 1;
				$nextStepNum = $currStepNum;
			} 
			elseif ($moveDir == 2) {
				if ($currStepNum == 1) {
					//boost move
					$nextStepNum = $gameRow['AreaSize'] + 1;
					$nextTrackNum = $currTrackNum;
					$boostMove = True;
				}
				else {
					//ordinary move - just decrement stackNum
					$nextStepNum = $currStepNum - 1;
					$nextTrackNum = $currTrackNum;
				}
			} 
			else {
				//decrement trackNum
				$nextTrackNum = $currTrackNum - 1;
				$nextStepNum = $currStepNum;
			}
		} 
		else {
			//player 1's move
			if ($moveDir == 3) {
				//decrement trackNum
				$nextTrackNum = $currTrackNum - 1;
				$nextStepNum = $currStepNum;
			} 
			elseif ($moveDir == 2) {
				if ($currStepNum == $gameRow['AreaSize']) {
					//boost move
					$nextStepNum = 0;
					$nextTrackNum = $currTrackNum;
					$boostMove = True;
				}
				else {
					//ordinary move - just increment stackNum
					$nextStepNum = $currStepNum + 1;
					$nextTrackNum = $currTrackNum;
				}
			} 
			else {
				//increment trackNum
				$nextTrackNum = $currTrackNum + 1;
				$nextStepNum = $currStepNum;
			} 
		}
		$nextStackNum = intval('8' . strval($nextTrackNum) . strval($nextStepNum));
		//find index of next stack record
		$nextStackRowsIndex = $nextStackNum;
		/*
		$nextStackRowsIndex = 0;
		$notFound = True;
		while ($notFound) {
			if ($stackRows[$nextStackRowsIndex]['StackNum'] == $nextStackNum) {
				$notFound = False; }
			else {
				$nextStackRowsIndex++; 
				if ($nextStackRowsIndex == $numStackRows) {
					$notFound = False;
					$errCode = "PMO-4";
					$errMsg = "Stack not found for stack num " . $nextStackNum;
					fnLogMessageToDb($errCode.$errMsg);
					$logMsgs[] = $errCode.$errMsg;
				}
			}
		} */
	}

	//only continue if there are no errors
	if (strlen($errCode) == 0) {
		if ($boostMove) {
			//move all counters
			$countToMove = $stackRows[$stackRowsIndex]['StackHeight']; }
		else {
			//move top counter only
			$countToMove = 1; }
		echo ' SRI=' . $stackRowsIndex . '< NSRI=' . $nextStackRowsIndex . '<';
		//move top counter(s) from current stack to next stack
		echo '<br>'.'curr counters='.$stackRows[$stackRowsIndex]['Counters'].'< len='.strlen($stackRows[$stackRowsIndex]['Counters']);
		//save top counter(s)
		$topCountersString = substr($stackRows[$stackRowsIndex]['Counters'], strlen($stackRows[$stackRowsIndex]['Counters'])-$countToMove, $countToMove);
		echo ' top counter(s)='.$topCountersString.'<';
		//remove top counter from current stack
		$stackRows[$stackRowsIndex]['Counters'] = substr($stackRows[$stackRowsIndex]['Counters'], 0, $stackRows[$stackRowsIndex]['StackHeight']-$countToMove);
		echo ' stack reduced to >' . $stackRows[$stackRowsIndex]['Counters'] . '<';
		//reduce height of current stack
		echo '<br>'.'heightt='.$stackRows[$stackRowsIndex]['StackHeight'];
		$stackRows[$stackRowsIndex]['StackHeight'] -= $countToMove;
		echo ' then reduces to '.$stackRows[$stackRowsIndex]['StackHeight'];
		//update owner of current stack
		echo '<br>'.'stack owner='.$stackRows[$stackRowsIndex]['OwnerPNum'];
		if ($boostMove) {
			$stackRows[$stackRowsIndex]['OwnerPNum'] =0; }
		else {
			$stackRows[$stackRowsIndex]['OwnerPNum'] = 
				intval(substr($stackRows[$stackRowsIndex]['Counters'], strlen($stackRows[$stackRowsIndex]['Counters'])-1, 1)); }
		echo ' then changed to '.$stackRows[$stackRowsIndex]['OwnerPNum'];
		//mark stack for db update
		$stackRows[$stackRowsIndex][0] = "Y";

		//increase height of next stack
		echo '<br>'.'next stack height='.$stackRows[$nextStackRowsIndex]['StackHeight'];
		$stackRows[$nextStackRowsIndex]['StackHeight'] += $countToMove;
		echo ' then changed to '.$stackRows[$nextStackRowsIndex]['StackHeight'];
		//update owner of next stack
		echo '<br>'.'next stack owner='.$stackRows[$nextStackRowsIndex]['OwnerPNum'];
		$stackRows[$nextStackRowsIndex]['OwnerPNum'] = intval(substr($topCountersString,strlen($topCountersString)-1,1));
		echo ' then changed to '.$stackRows[$nextStackRowsIndex]['OwnerPNum'];
		//add top counter(s) to next stack
		echo '<br>'.'next stack counters=',$stackRows[$nextStackRowsIndex]['Counters'].'<';
		if ($boostMove) {
			//make sure all counters belong to the owner
			$stackRows[$nextStackRowsIndex]['Counters'] = 
				str_repeat(strval($stackRows[$nextStackRowsIndex]['OwnerPNum']), $stackRows[$nextStackRowsIndex]['StackHeight']); }
		else {	
			$stackRows[$nextStackRowsIndex]['Counters'] .= $topCountersString; }
		echo '<br>'.' then changed to ',$stackRows[$nextStackRowsIndex]['Counters'].'<';
		//mark stack for db update
		$stackRows[$nextStackRowsIndex][0] = "Y";
		
	}
	
/*		if ($movePos == 0) {
			//Process move where player is using their reserve ";
			//check whose turn it is and adjust game step";
			if ($gameRow['LastPlayerID'] == $gameRow['P1RecID']) {
				//player 2's move";
				$stackNum = $gameRow['AreaSize'] + 1;
				$playerRef = $gameRow['P2RecID'];}
			else {
				// player 1's move";
				$stackNum = 0;
				$playerRef = $gameRow['P1RecID']; }
			//Subtract Move Num from the height of player X reserve";
			$stackRows[$stackNum]['StackHeight'] = $stackRows[$stackNum]['StackHeight'] - $moveNum;
			$stackRows[$stackNum][0] = "Y"; //mark for update

			//Reset the reserve to match the number of counters that remain by moving the Counters records that are no longer in the reserve to the target step in the playing area";
			$countersRows = fnQrySelectCounters($gameRef,$stackNum,0);
			if (is_string($countersRows)) {
				$errMsg = $countersRows ; //"Counters records not found. Fatal error";
				$errCode = "PMO-2A";
				fnLogMessageToDb($errCode.$errMsg);
				$logMsgs[] = $errCode.$errMsg; }
			else {
				for ($i = $stackRows[$stackNum]['StackHeight'] + 1; $i <= $stackRows[$stackNum]['StackHeight'] + $moveNum; $i++) {   
					//move the counter forward by updating the StepNum";
					if ($stackNum == 0) {
						//stackNum = 0 i.e. player 1";
						$countersRows[$i - 1]['StackNum'] = $stackNum + $targetStep; 
						$countersRows[$i - 1][0] = "Y"; //mark row for update
						$stackRows[$stackNum + $targetStep]['OwnerPNum'] = $gameRow['P1RecID'];   
						$stackRows[$stackNum + $targetStep][0] = "Y";} //mark row for update 
					else {
						//stackNum != 0 i.e. player 2";
						$countersRows[$i - 1]['StackNum'] = $stackNum - $targetStep;	
						$countersRows[$i - 1][0] = "Y"; //mark row for update
						$stackRows[$stackNum - $targetStep]['OwnerPNum'] = $gameRow['P2RecID']; 	
						$stackRows[$stackNum - $targetStep][0] = "Y";} 	//mark row for update 
					//put the counter rec on top of the stack by updating it's height
					$countersRows[$i - 1]['CounterHeight'] = $stackRows[$countersRows[$i - 1]['StackNum']]['StackHeight'] + $i - $stackRows[$stackNum]['StackHeight'] ;
				}
				//update the height of the game step";
				if ($stackNum == 0) {
					$stackRows[$stackNum + $targetStep]['StackHeight'] += $moveNum;}	
				else {
					$stackRows[$stackNum - $targetStep]['StackHeight'] += $moveNum;}	
			}
		}
		//process move in the play area
		else {
			//check whose turn it is and adjust game step 
			if ($gameRow['LastPlayerID'] == $gameRow['P1RecID']) {	
				//player 2's move
				$stackNum = $gameRow['AreaSize'] + 1 - $movePos;
				$playerRef = $gameRow['P2RecID']; }
			else {
				//player 1's move
				$stackNum = $movePos;
				$playerRef = $gameRow['P1RecID']; } 

			//get counter records
			$countersRows = fnQrySelectCounters($gameRef, $stackNum, 0);
			if (is_string($countersRows)) {
				$errMsg = "Counter records not found. Fatal error";
				$errCode = "PMO-2B";
				fnLogMessageToDb($errCode.$errMsg);
				//return array("errCode"=>$errCode,"errMsg"=>$errMsg);
				$logMsgs[] = $errCode.$errMsg; 
			}
			else {
				//take one or more counters away from the current step
				//reduce height of stack by number of counters being moved
				$stackRows[$stackNum]['StackHeight'] -= $moveNum ;
				$stackRows[$stackNum][0] = "Y"; //mark row for update 
				//change top counter	
				if ($stackRows[$stackNum]['StackHeight'] == 0) {
					//set top to zero
					$stackRows[$stackNum]['OwnerPNum'] = 0; }
				else {
					//set top to a number > zero
					$stackRows[$stackNum]['OwnerPNum'] = $countersRows[$stackRows[$stackNum]['StackHeight']-1]['PlayerRef']; }	

				//increase height of next stack, move counters and update top";
				if ($playerRef == $gameRow['P1RecID']) {
					// player 1";
					$stackRows[$stackNum+1][0] = "Y"; //mark row for update 		
					//move counters 
					for ($i = $stackRows[$stackNum]['StackHeight'] + 1; $i <= $stackRows[$stackNum]['StackHeight'] + $moveNum; $i++) {   
					$countersRows[$i-1]['StackNum'] = $stackNum + 1 ; 
					$countersRows[$i-1][0] = "Y"; //mark row for update
					$countersRows[$i-1]['CounterHeight'] = $stackRows[$stackNum+1]['StackHeight'] + $i - $stackRows[$stackNum]['StackHeight'] ; }
					//increase height
					$stackRows[$stackNum+1]['StackHeight'] += $moveNum ;	
					//update top of stack
					$stackRows[$stackNum+1]['OwnerPNum'] = $gameRow['P1RecID'] ; 
				}
				else {
					// player 2";	
					$stackRows[$stackNum-1][0] = "Y"; //mark row for update 	
					//move counters 
					for ($i = $stackRows[$stackNum]['StackHeight'] + 1; $i <= $stackRows[$stackNum]['StackHeight'] + $moveNum; $i++) {   
						$countersRows[$i-1]['StackNum'] = $stackNum - 1 ; 
						$countersRows[$i-1][0] = "Y"; //mark row for update
						$countersRows[$i-1]['CounterHeight'] = $stackRows[$stackNum-1]['StackHeight'] + $i - $stackRows[$stackNum]['StackHeight'] ; }
					//increase height
					$stackRows[$stackNum-1]['StackHeight'] += $moveNum  ;
					//update top of stack
					$stackRows[$stackNum-1]['OwnerPNum'] = $gameRow['P2RecID'] ; 
				}

				//If player has reached the far side of the board then all the counters are moved to the players reserve";
				if ($playerRef == $gameRow['P1RecID']) {
					//player 1
					if ($stackNum+1 == $gameRow['AreaSize']) {	
						//player 1 has reached the far side of the board so ...";
						//save layer updates
						$maxI = count($countersRows);
						for ($i=0; $i < $maxI; $i++) {
							if ($countersRows[$i][0] == "Y") {
								$fnResp = fnQryUpdateCounters($countersRows[$i]['GameRef'],$countersRows[$i]['StackNum'],$countersRows[$i]['PlayerRef'],$countersRows[$i]['CounterHeight'],$countersRows[$i]['CounterRecID']) ; 
								if (strlen($fnResp) > 0 ) {
									$errMsg = "Process Move - " . $fnResp;
									$errCode = "PMO-D";
									$conn->rollback(); 
									fnLogMessageToDb($errCode.$errMsg);
									//return array("errCode"=>$errCode,"errMsg"=>$errMsg);
									$logMsgs[] = $errCode.$errMsg; 
									break; 
								}
							}
						}
						if (strlen($errCode) == 0 ) {
							//select counters records for next game step
							$countersRows = fnQrySelectCounters($gameRow['GameRecID'], $stackNum+1, 0);
							if (is_string($countersRows)) {
								$errMsg = "Counter records not found. Fatal error";
								$errCode = "PMO-2C";
								//return array("errCode"=>$errCode,"errMsg"=>$errMsg); 
								$logMsgs[] = $errCode.$errMsg; }
							else {
								//move counters and make them belong to player 1
								for ($i=0; $i<=$stackRows[$stackNum+1]['StackHeight'] - 1; $i++) {		
									$countersRows[$i]['CounterHeight']= $stackRows[0]['StackHeight'] + 1 + $i;	
									$countersRows[$i]['StackNum'] = 0; 
									$countersRows[$i]['PlayerRef'] = $gameRow['P1RecID'];	
									$countersRows[$i][0] = "Y"; } 	//flag for update 			
								//increase height of reserve
								$stackRows[0]['StackHeight'] += $stackRows[$stackNum+1]['StackHeight'];
								$stackRows[0][0] = "Y";  //flag for update 				
								//reset height of far side of board
								$stackRows[$stackNum+1]['StackHeight'] = 0;
								$stackRows[$stackNum+1]['OwnerPNum'] = 0; 	
								$stackRows[$stackNum+1][0] = "Y";  //flag for update 
							}
						}
					}
				}
				else {
					//player 2
					if ($stackNum-1 == 1) {
						//player 2 has read the far side of the board so ...";
						//save layer updates
						$maxI = count($countersRows);
						for ($i=0; $i < $maxI; $i++) {
							if ($countersRows[$i][0] == "Y") {
								$fnResp = fnQryUpdateCounters($countersRows[$i]['GameRef'],$countersRows[$i]['StackNum'],$countersRows[$i]['PlayerRef'],$countersRows[$i]['CounterHeight'],$countersRows[$i]['CounterRecID']);
								if (strlen($fnResp) > 0 ) {
									$errMsg = "Process Move - " . $fnResp;
									$errCode = "PMO-E";
									$conn->rollback(); 
									fnLogMessageToDb($errCode.$errMsg);
									//return array("errCode"=>$errCode,"errMsg"=>$errMsg);
									$logMsgs[] = $errCode.$errMsg; 
									break;
								}
							}
						}
						if (strlen($errCode) == 0) {
							//select layer records for next game step";		
							$countersRows = fnQrySelectCounters($gameRow['GameRecID'], $stackNum-1,0);
							if (is_string($countersRows)) {
								$errMsg = "Counter records not found. Fatal error";
								$errCode = "PMO-2D";
								//return array("errCode"=>$errCode,"errMsg"=>$errMsg);
								$logMsgs[] = $errCode.$errMsg; 
							}
							else {
								//move counters and make them belong to player 2
								for ($i= 0; $i<= $stackRows[$stackNum-1]['StackHeight'] - 1; $i++) { 
									$countersRows[$i]['CounterHeight'] = $stackRows[$gameRow['AreaSize']+1]['StackHeight'] + 1 + $i; 	
									$countersRows[$i]['StackNum'] = $gameRow['AreaSize'] + 1;
									$countersRows[$i]['PlayerRef'] = $gameRow['P2RecID'];	
									$countersRows[$i][0] = "Y"; } 	//mark for update
								//increase height of reserve
								$stackRows[$gameRow['AreaSize']+1]['StackHeight'] += $stackRows[$stackNum-1]['StackHeight'];
								$stackRows[$gameRow['AreaSize']+1][0] = "Y";  //flag for update 			
								//reset height of far side of board
								$stackRows[$stackNum-1]['StackHeight'] = 0;	
								$stackRows[$stackNum-1]['OwnerPNum'] = 0;	
								$stackRows[$stackNum-1][0] = "Y";	//mark for update
							}
						}
					}
				}
			}
		} 
	} */
	
	//only continue if there are no errors
	//check to see if the next player has any moves available
	if (strlen($errCode) == 0) {
		if ($gameRecID == $gameRow['P1RecID']) {
			//P2 has moved so check if P1 has lost";
			if (fnCheckLoser(1, $stackRows, $gameRow)) {
			//if (fnCheckLoser($gameRow['P1RecID'], $stackRows)) {
				$gameRow['Winner'] = $gameRecID; $gameRow['LastPlayerID'] = $gameRow['P2RecID'];
				$errMsg2 = "Player " . $gameRecID . " has won the game."; 
				$errCode2 = "PMO-3"; 
				fnLogMessageToDb($errCode2.$errMsg2);
				$logMsgs[] = $errCode2.$errMsg2; }
			else {
				//P2 has moved but not won so check if P1 has any moves";
				//update game record so it looks like it is P1's turn
				$gameRow['LastPlayer'] = $gameRow['P2RecID'];
				//get a list of possible moves
				$fnResp = fnGetMoves(0, $gameRow, $stackRows);
				if (is_string($fnResp)) {
					$errCode = "PMO-F";
					$errMsg = $fnResp;
					fnLogMessageToDb($errCode.$errMsg); }
				else {
					/*
					$anyMoves = False;
					if ($stackRows[0]['StackHeight'] > 0) {
						$anyMoves = True;}
					else {
						for ($i = 1; $i<=$gameRow['AreaSize']-1; $i++) {	
							if ($stackRows[$i]['StackHeight'] >= $stackRows[$i+1]['StackHeight'] And $stackRows[$i]['OwnerPNum'] == $gameRow['P1RecID']) {
								$anyMoves = True;
								break; }
						}
					}*/
					//if ($anyMoves) {
					$movesList = $fnResp;
					echo 'number of moves for P1='.count($movesList).'<';
					if (count($movesList) > 0) {
						//P1 has moves available so leave LastPlayerID as P2
						//$gameRow['LastPlayerID'] = $gameRow['P2RecID'];
						}
					else {
						//P1 has no moves available so switch LastPlayerID so that it is P2's turn
						$gameRow['LastPlayerID'] = $gameRow['P1RecID'];
						$errMsg2 = "Player " . $gameRow['P1RecID'] . " has no moves available. Miss a turn."; 
						$errCode2 = "PMO-4"; 
						fnLogMessageToDb($errCode2.$errMsg2);
						$logMsgs[] = $errCode2.$errMsg2;
					}
				}
			}
		}
		else {
			//P1 has moved so check if P2 has lost";
			if (fnCheckLoser(2, $stackRows, $gameRow)) {
			//if (fnCheckLoser($gameRow['P2RecID'], $stackRows)) {	
				$gameRow['Winner'] = $gameRecID; $gameRow['LastPlayerID'] = $gameRow['P1RecID'];
				$errMsg2 = "Player " . $gameRecID . " has won the game."; 
				$errCode2 = "PMO-5"; 
				fnLogMessageToDb($errCode2.$errMsg2);
				$logMsgs[] = $errCode2.$errMsg2;
			}
			else {
				//P1 has moved but not won so check if P2 has any moves
				//update game record so it looks like it is P2's turn
				$gameRow['LastPlayer'] = $gameRow['P1RecID'];
				//get a list of possible
				$fnResp = fnGetMoves(0, $gameRow, $stackRows);
				if (is_string($fnResp)) {
					$errCode = "PMO-G";
					$errMsg = $fnResp;
					fnLogMessageToDb($errCode.$errMsg); }
				else {
					/*
					$anyMoves = False;
					if ($stackRows[$gameRow['AreaSize']+1]['StackHeight'] > 0) {
						$anyMoves = True; }
					else {
						for ($i=$gameRow['AreaSize']; $i>=2; $i--) { 
							if ($stackRows[$i]['StackHeight'] >= $stackRows[$i-1]['StackHeight'] And $stackRows[$i]['OwnerPNum'] == $gameRow['P2RecID']) {
								$anyMoves = True;
								break; }
						}
					} */
					//if ($anyMoves) {
					$movesList = $fnResp;
					if (count($movesList) > 0) {
						//P2 has moves available so leave LastPlayerID as P1
						//$gameRow['LastPlayerID'] = $gameRow['P1RecID']; 
					}
					else {
						//P2 has no moves available so switch LastPlayerID so that it is P1's turn
						$gameRow['LastPlayerID'] = $gameRow['P2RecID'];
						$errMsg2 = "Player " . $gameRow['P2RecID'] . " has no moves available. Miss a turn."; 
						$errCode2 = "PMO-6"; 
						fnLogMessageToDb($errCode2.$errMsg2);
						$logMsgs[] = $errCode2.$errMsg2;
					}
				}
			}
		}
	}
	
	/* //save counter recs
	if ($logLevel>2) {fnLogMessageToDb("updating counters") ;}
	$logMsgs[] = "updating counters";
	$maxI = count($countersRows);
	for ($i=0; $i<$maxI; $i++) {
		if ($countersRows[$i][0] == "Y") {
			$fnResp = fnQryUpdateCounters($countersRows[$i]['GameRef'],$countersRows[$i]['StackNum'],$countersRows[$i]['PlayerRef'],$countersRows[$i]['CounterHeight'],$countersRows[$i]['CounterRecID']);
			if (strlen($fnResp) > 0) {
				$errCode2 = "PMO-8";
				$errMsg2 = "maxI={$maxI} i={$i} GR={$countersRows[$i]['GameRef']} SN={$countersRows[$i]['StackNum']} PR={$countersRows[$i]['PlayerRef']} CH={$countersRows[$i]['CounterHeight']} CID={$countersRows[$i]['CounterRecID']}" . $fnResp; 
				fnLogMessageToDb($errCode2.$errMsg2);
				$logMsgs[] = $errCode2.$errMsg2;
				break; }
		}
	} */
	
	/* //only continue if there are no errors
	if (strlen($errCode2) == 0) {
		//save backup of counter records
		if ($logLevel>2) {fnLogMessageToDb("saving counters backup") ;}
		$logMsgs[] = "saving counters backup" ;
		//1. get all counters for this game and turncount 0
		$countersRows = fnQrySelectAllCounters($gameRef,0);
		if (is_string($countersRows)) {
			$errMsg2 = "Counter records not found. Fatal error";
			$errCode2 = "PMO-7";
			fnLogMessageToDb($errCode2.$errMsg2);
			//return array("errCode"=>$errCode2,"errMsg"=>$errMsg2); 
			$logMsgs[] = $errCode2.$errMsg2; }
		else {
			//2. save backup of this turncount
			$maxI = count($countersRows);
			for ($i=0; $i<$maxI; $i++) {
				$fnResp = fnQryAddCounter($countersRows[$i]['GameRef'],$countersRows[$i]['StackNum'],$countersRows[$i]['PlayerRef'],$countersRows[$i]['CounterHeight'], $gameRow['TurnCount']); 
				if (strlen($fnResp) > 0) {
					$errCode2 = "PMO-9";
					$errMsg2 = "Process Move - " . $fnResp; 
					fnLogMessageToDb($errCode2.$errMsg2);
					$logMsgs[] = $errCode2.$errMsg2;
					break; }
			}
		}
	} */
	
	//only continue if there are no errors
	//save game rec
	if (strlen($errCode) == 0) {
		if ($logLevel>2) {fnLogMessageToDb("saving game") ;}
		$logMsgs[] = "saving game";
		//update the last updated time";
		$gameRow['LastUpdated'] = date("Y-m-d H:i:s");
		//increment move count
		$gameRow['TurnCount'] += 1;	
		$fnResp = fnQryUpdateGame($gameRow['GameRef'],$gameRow['P1RecID'],$gameRow['P2RecID'],$gameRow['LastPlayerID'],$gameRow['StartDate'],$gameRow['LastUpdated'],$gameRow['AreaSize'],$gameRow['Winner'],$gameRow['TurnCount']);
		if (strlen($fnResp) > 0) {
			$errCode = "PMO-0";
			$errMsg = $fnResp; 
			fnLogMessageToDb($errCode.$errMsg);
			$logMsgs[] = $errCode.$errMsg;
		}
		else {
			// also store a backup
			$fnResp = fnQryAddGame($gameRow['GameRef'],$gameRow['P1RecID'],$gameRow['P2RecID'],$gameRow['LastPlayerID'],$gameRow['StartDate'],$gameRow['LastUpdated'],$gameRow['AreaSize'],$gameRow['AreaWidth'],$gameRow['Winner'],$gameRow['TurnCount'],$gameRow['TurnCount']);
			if (strlen($fnResp) > 0) {
				$errCode = "PMO-A";
				$errMsg = $fnResp; 
				fnLogMessageToDb($errCode.$errMsg);
				$logMsgs[] = $errCode.$errMsg;
			}
		}
	}
	//only continue if there are no errors
	//save stack recs
	if (strlen($errCode) == 0) {
		if ($logLevel>2) {fnLogMessageToDb("saving stacks") ;}
		$logMsgs[] = "saving stacks";
		/*
		$maxI = count($stackRows);
		for ($i=0; $i<$maxI; $i++) { */
		for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
			for ($stepNum = 0;
					$stepNum < $gameRow['AreaSize']+2;
					$stepNum++) {
				//construct index
				$i = intval('8' . $trackNum . $stepNum);							
				echo 'contructed $i on line 1020=' . $i . '<';
				if ($stackRows[$i][0] == "Y") {
					$fnResp = fnQryUpdateStack($stackRows[$i]['StackNum'],$stackRows[$i]['StackHeight'],$stackRows[$i]['OwnerPNum'],$stackRows[$i]['Counters'],$stackRows[$i]['StackRecID']); 
					if (strlen($fnResp) > 0) {
						//if the stack number is the first or last and its error2 (no updates) then ignore the error
						if ((($stackRows[$i]['StackNum']==1) Or ($stackRows[$i]['StackNum']==$gameRow['AreaSize'])) And (stripos($fnResp, 'error2')>=0)) {}
						else {
							$errCode = "PMO-B";
							$errMsg = $fnResp; 
							fnLogMessageToDb($errCode.$errMsg);
							$logMsgs[] = $errCode.$errMsg;
							break; 
						}
					}
				}
				//also store a backup
				$fnResp = fnQryAddStack($stackRows[$i]['GameRef'],$stackRows[$i]['StackNum'],$stackRows[$i]['StackHeight'],$stackRows[$i]['OwnerPNum'],$stackRows[$i]['Counters'],$gameRow['TurnCount']);
				if (strlen($fnResp) > 0) {
					$errCode = "PMO-C";
					$errMsg = $fnResp; 
					fnLogMessageToDb($errCode.$errMsg);
					$logMsgs[] = $errCode.$errMsg;
					break; 
				}
			}
		}
	}
	//end database transaction
	if (strlen($errCode)>0) {
		//an error has occurred rollback any database uopdates
		$conn->rollback(); 
		foreach ($logMsgs as $msgs) {fnLogMessageToDb($msgs);}
		fnLogMessageToDb ("Process Move - Rollback");
		return array("errCode"=>$errCode,"errMsg"=>$errMsg); 
	}
	else {
		$conn->commit(); 
		fnLogMessageToDb ("Process Move - Commit");
		return array("errCode"=>$errCode2,"errMsg"=>$errMsg2); 
	}
}
//End fnProcessMove
function fnCopyGame($gameRef, $turnCount){
	fnLogMessageToDb("start fnCopyGame");
	/*
	make a complete copy of a game including stacks a counters records. 
	use a new game ref and set turncount to zero
	*/
	global $conn, $logLevel;

	//begin database transaction
	$conn->beginTransaction();
	$logMsgs=array("rewrite log msgs after rollback");

	$gameRow = fnQrySelectGame($gameRef);
	if (is_string($gameRow)) {
		$errMsg = $gameRow;
		$errCode = "CGR-1";
		fnLogMessageToDb($errCode.$errMsg);
		$logMsgs[] = $errCode.$errMsg; }
	else {
		//get stacks
		$stackRows = fnQrySelectStacks($gameRef, $turnCount );
		if (is_string($stackRows)) {
			$errMsg = $stackRows ;
			$errCode = "CGR-2";
			fnLogMessageToDb($errCode.$errMsg);
			$logMsgs[] = $errCode.$errMsg;	}
		else {
			//get counters
			$countersRows = fnQrySelectAllCounters($gameRef, $turnCount );
			if (is_string($countersRows)) {
				$errMsg = $counterRows;
				$errCode = "CGR-3";
				fnLogMessageToDb($errCode.$errMsg);
				$logMsgs[] = $errCode.$errMsg; }
			else {
				//already have a player ref so just get new game ref
				$refsRow = fnReferencesTable("new game ref");
				if (is_array($refsRow)) {
					$newGameRef = $refsRow['NextGameRef'];	}
				else {
					$errMsg = "error setting up new game reference";
					$errCode = "CGR-1";
					fnLogMessageToDb($errCode.$errMsg); 	
					$logMsgs[] = $errCode.$errMsg; }
			}
		}
	}
	//only continue if there are no errors
	if (strlen($errCode) == 0) {
		//save counters recs with new game ref and zero turn count
		if ($logLevel>2) {fnLogMessageToDb("copying counters") ;}
		$logMsgs[] = "copying counters" ;
		$maxI = count($countersRows);
		for ($i=0; $i<$maxI; $i++) {
			$fnResp = fnQryAddCounter($newGameRef,$countersRows[$i]['StackNum'],$countersRows[$i]['PlayerRef'],$countersRows[$i]['CounterHeight'],0); 
			if (strlen($fnResp) > 0) {
				$errCode = "CGR-2";
				$errMsg = $fnResp; 
				fnLogMessageToDb($errCode.$errMsg);
				$logMsgs[] = $errCode.$errMsg;
				break; }
		}
	}
	//only continue if there are no errors
	if (strlen($errCode) == 0) {
		//copy game with new game ref and zero turn count
		if ($logLevel>2) {fnLogMessageToDb("copying game") ;}
		$logMsgs[] = "copying game" ;
		$fnResp = fnQryAddGame($newGameRef,$gameRow['P1RecID'],$gameRow['P2RecID'],$gameRow['LastPlayerID'],date("Y-m-d H:i:s"),date("Y-m-d H:i:s"),$gameRow['AreaSize'],$gameRow['Winner'],0,0);
		if (strlen($fnResp) > 0) {
			$errCode = "CGR-3";
			$errMsg = $fnResp; 
			fnLogMessageToDb($errCode.$errMsg);
			$logMsgs[] = $errCode.$errMsg;
		}
	}
	//only continue if there are no errors
	if (strlen($errCode) == 0) {
		//copy stack recs with new game ref and zero turn count
		if ($logLevel>2) {fnLogMessageToDb("copying stacks") ;}
		$logMsgs[] = "copying stacks";
		$maxI = count($stackRows);
		for ($i=0; $i<$maxI; $i++) {
			$fnResp = fnQryAddStack($newGameRef,$stackRows[$i]['StackNum'],$stackRows[$i]['StackHeight'],$stackRows[$i]['OwnerPNum'],0);
			if (strlen($fnResp) > 0) {
				$errCode2 = "CGR-C";
				$errMsg2 = $fnResp; 
				fnLogMessageToDb($errCode.$errMsg);
				$logMsgs[] = $errCode.$errMsg;
				break; 
			}
		}
	}
	//end database transaction
	if (strlen($errCode)>0) { 
		//an error has occurred rollback any database updates
		$conn->rollback(); 
		foreach ($logMsgs as $msgs) {fnLogMessageToDb($msgs);}
		fnLogMessageToDb ("Copy game records - Rollback");
		return $errCode.$errMsg ; 
	}
	else {
		$conn->commit(); 
		fnLogMessageToDb (" Copy game records - Commit");
		return ""; 
	}
}
//End fnCopyGame
function fnEncodeBoard($stackRows) {
	//takes a set of game stacks and encodes them as a board for stoarge in a player table
	//board is a string of stackNum, ownerPNum and stackHeight delimited by 'x'. This ie repeated for every stack on the board
	fnLogMessageToDb('start of fnEncodeBoard');
	$board = "";
	
	/*
	**** Needs revising becuase index is now stackNum **** 
	for ($stackRowsIndex = 0; $stackRowsIndex < count($stackRows); $stackRowsIndex++) {
		$board .= $stackRows[$stackRowsIndex]['StackNum'].'x'.$stackRows[$stackRowsIndex]['OwnerPNum'].'x'.$stackRows[$stackRowsIndex]['StackHeight'].'x';
	} */
	//trim trailing 'x'
	$board = array_slice($board,0,-1);
	
	fnLogMessageToDb('exiting fnEncodeBoard. resp ='.$board);
	return $board;
}
//End fnEncodeBoard
function fnGetMoves($gameKey, $gameRow, $stackRows) {
	/*
	get all the possible moves for the given game for the next player
	return the result as a list (array) of moves in the form (GameRecID, MoveDir, MovePos) 
	MoveDir	= direction where 1 is left, 2 is forward and 3 is right
	MovePos = stack number where move starts
	*/
	
	fnLogMessageToDb ("start fnGetMoves. GK= {$gameKey}");
	global $logLevel;

	//an array to hold all the possible moves
	$movesList = array();
	$errCode=""; $errMsg="";

	//only need to do the following if a game key has been provided. If not the game and stacks data will be provided instead.
	if ($gameKey>0) {
		//find out if gameKey is GameRecID or Nmx hash key
		if (strlen($gameKey)<20) {
			$gameRecID=$gameKey; }
		else {
			$gameRecID = 0;
			$hashKey = $gameKey; }
			
		if ($gameRecID>0) {
			//get game record using GameRecID
			fnLogMessageToDb("get game record using GameRecID");
			$gameRow = fnQrySelectGameByID($gameRecID);
			if (is_string($gameRow)) {
				$errCode = "GM-4";
				$errMsg = $gameRow; 
				fnLogMessageToDb ($errCode.$errMsg); }
		}
		else {
			//get the relevant game record using hash key
			fnLogMessageToDb("get game record using hashKey");
			$gameRow = fnQrySelectNmxGame($hashKey);
			if (is_string($gameRow)) {
				$errCode = "GM-5";
				$errMsg = $gameRow;	
				fnLogMessageToDb ($errCode.$errMsg); }
		}

		if (strlen($errCode)==0){
			//check player 2 has been set up
			if ($gameRow['P2RecID']==0) {
				$errMsg = "Game " . $gameKey . " not started";
				$errCode = "GM-2";
				fnLogMessageToDb($errCode.$errMsg); }
			else {
				if ($gameRecID>0) {
					//get stack records using rec id
					$stackRows = fnQrySelectStacks($gameRow['GameRef'],$gameRow['TurnCount']);
					if (is_string($stackRows)) {
						$errMsg = "stacks not found. Fatal error.";
						$errCode = "GM-3";
						fnLogMessageToDb($errCode.$errMsg); }
				} 
				else {
					//get stacks record using hashkey
					$stackRows = fnQrySelectNmxStacks($hashKey);
					if (is_string($stackRows)) {
						$errMsg = "Stack not found";
						$errCode = "GM-6";
						fnLogMessageToDb($errCode.$errMsg); }
				}
			}
		}
	}
			
	if (strlen($errCode)==0) {
		//loop to find all the possible moves from the reserve
		if ($gameRow['LastPlayerID'] == $gameRow['P1RecID']) {
			//P2's turn. 
			//P2's reserves are the ones where the stepNum is AreaSize+1 * 1 to AreaWidth
			$stepNum = $gameRow['AreaSize']+1;
			for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
				//construct index
				$stackRowsIndex = intval('8' . $trackNum . $stepNum);
				//check if there is move going left, forward or right
				for ($moveDir = 1; $moveDir < 4; $moveDir++) {
					$fnResp = fnFindNextStack($gameRow, $stackRows, $stackRowsIndex, $moveDir, 2);
					//echo "fnFindNextStack resp ".intval($fnResp[0])."<BR>";
					if ($fnResp[0] == '-2'){
						//error in function
						$errCode = "GM-7";
						$errMsg = $fnResp[1].$fnResp[2]; }
					elseif (intval($fnResp[0]) > -1) {
						array_push($movesList, array($gameKey, $moveDir, $stackRows[$stackRowsIndex]['StackNum'])); 
						if ($logLevel>2) {fnLogMessageToDb("P2 possible reserve move: MD={$moveDir} MP={$stackRows[$stackRowsIndex]['StackNum']}"); }
					}
				}
			}
			/*
			//P2's reserves are the ones that is at index AreaSize+1 * 1 to AreaWidth
			for ($stackRowsIndex = $gameRow['AreaSize']+1; 
					$stackRowsIndex <= ($gameRow['AreaSize']+2) * $gameRow['AreaWidth']; 
					$stackRowsIndex += $gameRow['AreaSize']+2 ) {
				//check if there is move going left, forward or right
				for ($moveDir = 1; $moveDir < 4; $moveDir++) {
					$fnResp = fnFindNextStack($gameRow, $stackRows, $stackRowsIndex, $moveDir, 2);
					//echo "fnFindNextStack resp ".intval($fnResp[0])."<BR>";
					if ($fnResp[0] == '-2'){
						//error in function
						$errCode = "GM-7";
						$errMsg = $fnResp[1].$fnResp[2]; }
					elseif (intval($fnResp[0]) > -1) {
						array_push($movesList, array($gameKey, $moveDir, $stackRows[$stackRowsIndex]['StackNum'])); 
						if ($logLevel>2) {fnLogMessageToDb("P2 possible reserve move: MD={$moveDir} MP={$stackRows[$stackRowsIndex]['StackNum']}"); }
					}
				}
			} */
		}
		else {
			//P1's turn. 
			//P1's reserves are the ones where the stepNum is AreaSize+2 * 0 to AreaWidth-1
			for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
				for ($stepNum = 0; 
						$stepNum <= ($gameRow['AreaSize']+2) * ($gameRow['AreaWidth']-1); 
						$stepNum += $gameRow['AreaSize']+2 ) {
					//construct index
					$stackRowsIndex = intval('8' . $trackNum . $stepNum);
					for ($moveDir = 1; $moveDir < 4; $moveDir++) {
						$fnResp = fnFindNextStack($gameRow, $stackRows, $stackRowsIndex, $moveDir, 1);
						//echo "fnFindNextStack resp ".intval($fnResp[0])."<BR>";
						if ($fnResp[0] == -2){
							//error in function
							$errCode = "GM-8";
							$errMsg = $fnResp[1].$fnResp[2]; }
						elseif (intval($fnResp[0]) > -1) {
							array_push($movesList, array($gameKey, $moveDir, $stackRows[$stackRowsIndex]['StackNum']));  
							if ($logLevel>2) {fnLogMessageToDb("P1 possible reserve move: MD={$moveDir} MP={$stackRows[$stackRowsIndex]['StackNum']}"); }
						}
					}
				}
			}
			/*
			//P1's reserves are the ones that is at index AreaSize+2 * 0 to AreaWidth-1
			for (	$stackRowsIndex = 0; 
					$stackRowsIndex <= ($gameRow['AreaSize']+2) * ($gameRow['AreaWidth']-1); 
					$stackRowsIndex += $gameRow['AreaSize']+2 ) {
					//check if there is move going left, forward or right
				for ($moveDir = 1; $moveDir < 4; $moveDir++) {
					$fnResp = fnFindNextStack($gameRow, $stackRows, $stackRowsIndex, $moveDir, 1);
					//echo "fnFindNextStack resp ".intval($fnResp[0])."<BR>";
					if ($fnResp[0] == -2){
						//error in function
						$errCode = "GM-8";
						$errMsg = $fnResp[1].$fnResp[2]; }
					elseif (intval($fnResp[0]) > -1) {
						array_push($movesList, array($gameKey, $moveDir, $stackRows[$stackRowsIndex]['StackNum']));  
						if ($logLevel>2) {fnLogMessageToDb("P1 possible reserve move: MD={$moveDir} MP={$stackRows[$stackRowsIndex]['StackNum']}"); }
					}
				}
			} */
		}
	}
	//echo "errcode after reserve checks=".$errCode."<"."<BR>";
	if (strlen($errCode)==0){
		//loop to find all the possible moves in the playing area
		if ($gameRow['LastPlayerID'] == $gameRow['P1RecID']) {
			//P2's turn
			//check all the the tracks
			for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
				/*
				for ($stackRowsIndex = (($gameRow['AreaSize']+2) * $trackNum) + 1; 
						$stackRowsIndex < ($gameRow['AreaSize']+1) * ($trackNum+1);
						$stackRowsIndex++) { */
				//check each step in the playing area (skip the reserve stacks)
				for ($stepNum = 1;
						$stepNum <= $gameRow['AreaSize'] + 1;
						$stepNum++) {
					//construct index
					$stackRowsIndex = intval('8' . $trackNum . $stepNum);
					//check stack belongs to P2 otherwise do nothing
					if ($stackRows[$stackRowsIndex]['OwnerPNum'] == 2) {
						//check if there is move going left, forward or right
						for ($moveDir = 1; $moveDir < 4; $moveDir++) {
							$fnResp = fnFindNextStack($gameRow, $stackRows, $stackRowsIndex, $moveDir, 2);
							//echo "fnFindNextStack resp ".$fnResp[0]."<BR>";
							if ($fnResp[0] == -2){
								//error in function
								$errCode = "GM-9";
								$errMsg = $fnResp[1].$fnResp[2]; }
							elseif (intval($fnResp[0]) > -1) {
								array_push($movesList, array($gameKey, $moveDir, $stackRows[$stackRowsIndex]['StackNum'])); 
								if ($logLevel>2) {fnLogMessageToDb("P2 possible move: MD={$moveDir} MP={$stackRows[$stackRowsIndex]['StackNum']}"); }	
							}
						}
					}
				}
			}
		}
		else {
			//echo "P1"."<BR>";
			//P1's turn
			//check all the the tracks
			for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
				//check each step in the playing area (skip the reserve stacks)
				/*
				for (	$stackRowsIndex = (($gameRow['AreaSize']+2) * $trackNum) + 1; 
						$stackRowsIndex < ($gameRow['AreaSize']+1) * ($trackNum+1);
						$stackRowsIndex++) { */
				for ($stepNum = 1; 
						$stepNum <= $gameRow['AreaSize'] + 1;
						$stepNum++) {
					//construct index
					$stackRowsIndex = intval('8' . $trackNum . $stepNum);
					//check stack belongs to P1 otherwise do nothing
					if ($stackRows[$stackRowsIndex]['OwnerPNum'] == 1) {
						//check if there is move going left, forward or right
						for ($moveDir = 1; $moveDir < 4; $moveDir++) {
							$fnResp = fnFindNextStack($gameRow, $stackRows, $stackRowsIndex, $moveDir, 1);
							//echo "fnFindNextStack resp ".$fnResp[0]."<BR>";
							if ($fnResp[0] == -2){
								//error in function
								$errCode = "GM-A";
								$errMsg = $fnResp[1].$fnResp[2]; }
							elseif (intval($fnResp[0]) > -1) {
								array_push($movesList, array($gameKey, $moveDir, $stackRows[$stackRowsIndex]['StackNum'])); 
								if ($logLevel>2) {fnLogMessageToDb("P2 possible move: MD={$moveDir} MP={$stackRows[$stackRowsIndex]['StackNum']}"); }
							}
						}
					}
				}
			}
		}
	}
	
	if (strlen($errCode)==0){
		fnLogMessageToDb('fnGetMoves ending. Moves found=' . count($movesList) . '. Response=' . print_r($movesList,True));
		return $movesList; }
	else {
		fnLogMessageToDb('fnGetMoves ending. Response='.$errCode.$errMsg);
		return $errCode.$errMsg; }
}
//End fnGetMoves
function fnFindNextStack($gameRow, $stackRows, $stackRowsIndex, $moveDir, $playerNum) {
	fnLogMessageToDb('start fnFindNextStack. SRI=' . $stackRowsIndex . ' MD=' .	$moveDir . ' PN' . $playerNum);
	/* checks to see if it is possible to move in a particular directions
	stackRows = list of all the stacks in the game area (including reserves)
	stackRowsIndex = index of the stack where the move is to start from
	moveDir = direction of the move from the player's perspective. 1 = left, 2 = forward, 3 = right
	playerNum = player to take move. check that stack belongs to this player otherwise return an error
	
	returns an array with 3 parts:
	[0] = -2 if there is an error, -1 if move not possible or zero if a move is possible
	[1] = error code or empty string
	[2] = error message or empty string
	*/
	//set up response value
	$fnResp = array("0","","");
	
	//step num is third and any subsequent digits of the stack num
	$currStepNum = intval(substr((string)$stackRows[$stackRowsIndex]['StackNum'],2,strlen($stackRows[$stackRowsIndex]['StackNum'])-2));
	//echo "<br>"." CS:".$currStepNum;
	//track is second digit of the stack num
	$currTrackNum = intval(substr((string)$stackRows[$stackRowsIndex]['StackNum'],1,1));
	//echo " CT:".$currTrackNum." MD".$moveDir."  ";
	
	//check player num
	if ($stackRows[$stackRowsIndex]['OwnerPNum'] != $playerNum) {
		$fnResp[0] = '-2';
		$fnResp[1] = 'FN-1';
		$fnResp[2] = 'Stack '. $stackRows[$stackRowsIndex]['StackNum'] . 'does not belong to player '.$playerNum;
		fnLogMessageToDb($fnResp[1].$fnResp[2]);
	}
	
	if ($fnResp[0] == "0") {
		//check if move direction is valid (ie. that it does not go outside of the game area) 
		if ($playerNum == 1) {
			// P1 moves left to right so first track (track 0) cannot move left and last track cannot move right
			if (	($moveDir == 1 and $currTrackNum == 0) or
					($moveDir == 3 and $currTrackNum == $gameRow['AreaWidth'] - 1 )) {
				$fnResp[0] = '-1'; }
			/* P1 cannot move into P2 reserve
			elseif ($moveDir == 2 and  $currStepNum == $gameRow['AreaSize']) {
				$fnResp[0] = '-1';} */
		}
		else {
			// P2 moves right to left so first track (track 0) cannot move right and last track cannot move left
			if (	($moveDir == 3 and $currTrackNum == 0) or
					($moveDir == 1 and $currTrackNum == $gameRow['AreaWidth'] - 1 )) {
				$fnResp[0] = '-1'; }
			/* P2 cannot move into P1 reserve
			elseif ($moveDir == 2 and  $currStepNum == 1) {
				$fnResp[0] = '-1';} */
		}
	}
	/*
	if ($fnResp[0] == "0") {
		//work out which stack the player wants to move to

		if ($playerNum == 1) {
			//P1 move left
			if ($moveDir == 1) {
				//decrement track number by 1
				$nextTrackNum = $currTrackNum - 1;
				//change track into a stack num
				$nextStackNum = intval('8' . strval($nextTrackNum) . strval($currStepNum));
			}
			//P1 forward move
			if ($moveDir == 2) {
				//increment current step by 1
				$nextStepNum = $currStepNum + 1;
				//change step into a stack num
				$nextStackNum = intval('8' . strval($currTrackNum) . strval($nextStepNum));
			}
			//P1 move right
			if ($moveDir == 3) {
				//increment track number by 1
				$nextTrackNum = $currTrackNum + 1;
				//change track into a stack num
				$nextStackNum = intval('8' . strval($nextTrackNum) . strval($currStepNum));
			}

		} else {
			//P2 move left
			if ($moveDir == 1) {
				//increment track number by 1
				$nextTrackNum = $currTrackNum + 1;
				//change track into a stack num
				$nextStackNum = intval('8' . strval($nextTrackNum) . strval($currStepNum));
			}
			//P2 forward move
			if ($moveDir == 2) {
				//decrement current step by 1
				$nextStepNum = $currStepNum - 1;
				//change step into a stack num
				$nextStackNum = intval('8' . strval($currTrackNum) . strval($nextStepNum));
			}
			//P2 move right
			if ($moveDir == 3) {
				//decrement track number by 1
				$nextTrackNum = $currTrackNum - 1;
				//change track into a stack num
				$nextStackNum = intval('8' . strval($nextTrackNum) . strval($currStepNum));
			}
		}
			
		//the rules for moving are the same for the whole board (including the reserves)
		//loop through stack rows to find one with required stack num
		$index = 0;
		$numStackRows = count($stackRows);
		$notFound = True;
		while ($notFound) {
			if ($stackRows[$index]['StackNum'] == $nextStackNum) {
				$notFound = False;
				$fnResp[0] = strval($index); }
			else {
				$index++; 
				if ($index == $numStackRows) {
					$notFound = False;
					$fnResp[0] = '-2';
					$fnResp[1] = "FN-2";
					$fnResp[2] = "Stack not found for stack num " . $nextStackNum;
					fnLogMessageToDb($fnResp[1].$fnResp[2]);
				}
			}
		}
	} */
	fnLogMessageToDb("fnFindNextStack ending. Response=".print_r($fnResp,True));
	return $fnResp;
}
//End fnFindNextStack
function fnCheckLoser($playerNum, $stackRows, $gameRow) {
	fnLogMessageToDb("start fnCheckLoser");
	//returns True if the given player has lost otherwise False
	$fnResp = True;
	//not a loser if there is something in reserve
	if ($playerNum == 1) {
		//check P1 reserves
		$stepNum = 0;
		for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
			//construct index
			$stackRowsIndex = intval('8' . $trackNum . $stepNum);
			if ($stackRows[$stackRowsIndex]['StackHeight'] > 0) {
				 $fnResp = False; }
		}
	}
	else {
		//check P2 reserves
		$stepNum = $gameRow['AreaSize']+1; 
		for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
			//construct index
			$stackRowsIndex = intval('8' . $trackNum . $stepNum);
			if ($stackRows[$stackRowsIndex]['StackHeight'] > 0) {
				$fnResp = False; }
		}
	}

	if ($fnResp) {
		//not a loser if player has a counter on top of one of the game steps
		//check all the the tracks
		for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
			//check each step in the playing area (skip the reserve stacks)
			for ($stepNum = (($gameRow['AreaSize']+2) * $trackNum) + 1; 
					$stepNum < ($gameRow['AreaSize']+1) * ($trackNum+1);
					$stepNum++) {
				//construct index
				$stackRowsIndex = intval('8' . $trackNum . $stepNum);							
				if ($playerRef == $stackRows[$stackRowsIndex]['OwnerPNum']) {
					$fnResp = False; }
			}
		}
	}
	
	if ($fnResp) {
		fnLogMessageToDb('ending fnCheckLoser. Response = True'); }
	else {
		fnLogMessageToDb('ending fnCheckLoser. Response = False'); }
	
	return $fnResp;
}
//End fnCheckLoser
function fnProcessInput($callString) {
	fnLogMessageToDb("start fnProcessInput");
	//globals
	global $tbP1NewPlyrName, $tbP1NewGameRef;
	global $tbP2NewPlyrName, $tbP2NewGameRef; 
	global $conn, $logLevel;
	
	//convert call string to upper case
	$callString = strtoupper($callString);
	
	//call function to extract the parameters from the call string
	$refs=fnFindParams($callString);
	//print_r($refs);

	$errCode = ""; $errMsg = "";
	if (array_key_exists('errCode',$refs) == True) {
		$errCode = $refs['errCode'] ;
        $errMsg = $refs['errMsg'] ; }
	elseif (array_key_exists("TYPE",$refs) == False) {
		$errCode = "PrI-1";
		$errMsg = "input string - type param missing"; }#
    else {
		switch ($refs["TYPE"]){
			case "TESTING":
				//$gameRow= fnQrySelectNmxGame($refs['HASHKEY']);
				//echo fnQryUpdateNmxGame($gameRow['HashKey'],$gameRow['P1RecID'],$gameRow['P2RecID'],$gameRow['LastPlayerID'],$gameRow['AreaSize'],$gameRow['Winner'],$gameRow['TurnCount'], 5.5);
				
				//$errMsg = fnGetMoves($refs['GAMERECID'],[],[]);
				echo '<BR>';
				$errMsg = fnProcessMove(11,1,815);
				break;

			case 'COPY':
			case 'COPYGAME':
				print(fnCopyGame($refs['GAMEREF'], $refs['TURNCOUNT']));
				break;
				
			case "GET":
			case "GETMOVES":
				print_r(fnGetMoves($refs["GAMEREF"],[],[]));
				break;
		
			case "DATACHECK":
			case "CHECK":
				fnDataChecks($refs["GAMEREF"]);
				break;

			case "P1_NEW_GAME":
			case "P1NEWGAME":
				//player name and game central are both optional parameters
                if (count($refs) > 3) {
                    //error - too many parameters
					$errCode = "P1N-1";
					$errMsg = "P1 New Player - too many parameters";} 
				elseif (count($refs) == 1) {
					//error - no parameters
					$errCode = "P1N-4";
					$errMsg ="P1 New Player - no parameters"; }	
				elseif (count($refs) == 2) {
					//if there is one parameter then it must be Player Name
                    if (array_key_exists("PLAYERNAME",$refs) == False ) {
                        //error - only valid parameter is player name
						$errCode = "P1N-2";
                        $errMsg = "P1 New Player - invalid parameters"; } }
				elseif ((array_key_exists("PLAYERNAME",$refs) == False ) or (array_key_exists("GAMECENTRAL",$refs) == False )) {
					//error - invalid parameters
					$errCode = "P1N-3";
					$errMsg ="P1 New Player - invalid parameters"; }
				elseif (array_key_exists("GAMECENTRAL",$refs) == True And $refs["GAMECENTRAL"] <> "Y" And $refs["GAMECENTRAL"] <>  "N") {
					//error game central must by y or n
					$errCode = "P1N-8";
					$errMsg ="P1 New Player - game central parameter must by y or n"; }
				
				if (strlen($errCode) == 0) {
					//valid input so carry on
					if (count($refs) == 2) {
						//one parameter which must be the player name. Set gameCentral to No
                        $gameCentral = "N";
						$tbP1NewPlyrName = $refs["PLAYERNAME"]; }
					else {
						//two parameters for both game central and player name
						fnLogMessageToDb("P1 new game params - player name =" . $refs["PLAYERNAME"] . " and game ref =" . $refs["GAMECENTRAL"]);
						$gameCentral = $refs["GAMECENTRAL"];
						$tbP1NewPlyrName = $refs["PLAYERNAME"];}
						
					//check the password
					$fnResp = fnValidatePlayerNamePW($tbP1NewPlyrName); 
					if (substr($fnResp,0, 3) == 'VP-') {
						$errCode = 'P1N-7';
						$errMsg = $fnResp; } 
					else {
						//extract player name
						$inx = strpos($tbP1NewPlyrName,"/",0); 
						$tbP1NewPlyrName = substr($tbP1NewPlyrName,0,$inx); } 
				}
				if (strlen($errCode) == 0) {
					$fnResp = fnProcessNewP1($tbP1NewPlyrName, True); 
					if (array_key_exists('errCode',$fnResp) == False) {
                        //Response from fnProcessNewP1 is ok - a numeric fnResp means the new game ref has been returned instead of string error message
						$gameRef = $fnResp['gameRef'];
						//call state of play 
						$fnResp = fnStateOfPlay($gameRef);
						if (is_array($fnResp)) {
							$errCode = "    ";
                            $errMsg = json_encode($fnResp); }
                        else {
							$errCode = 'P1N-5';
                            //$errMsg = "P1 New Player - error response from state of play"; } }
							$errMsg = $fnResp; }
					}
					else {
						//error occurred in fnProcessNewP1
                        $errCode = 'P1N-6';
                        //$errMsg = "P1 New Player - error response from process new player 1"; } 
						$errMsg = $fnResp['errCode'] . $fnResp['errMsg']; }
						
					//if state of play has been called successfully then check GAMECENTRAL flag and set up player 2 if required
					if ($errCode == "    " and $gameCentral == "Y") {
						$fnResp = fnProcessNewP2($gameRef, "GAMECENTRAL", True);
						if (strlen($fnResp) > 0) {
							//errors occurred
							$errCode = substr($fnResp,0, 5);
							$errMsg = "error occurred setting up GAMECENTRAL as player 2 [" . substr($fnResp,5,strlen($fnResp)-5) . "]"; } 
						else {
							//call state of play 
							$fnResp = fnStateOfPlay($gameRef);
							if (is_array($fnResp)) {
								$errCode = "    ";
								$errMsg = json_encode($fnResp); }
							else {
								$errCode = "P1N-9";
								$errMsg = "P1 New Player - error response from state of play (for player 2)"; }
						}
					}
				}
				break;
			
			case "P2_NEW_GAME":
			case "P2NEWGAME":
                //must have parameters for gameRef and playerName
                if (count($refs) != 3) {
					//error - incorrect number of parameters
                    $errCode = "P2N-1";
                    $errMsg = "P2 New Player - incorrect number of parameters"; }
                else {
					//2 parameters - one must be game ref and the other player name. 
					if (array_key_exists("GAMEREF",$refs) == False) {
						//error - game ref parameter missing
						$errCode = "P2N-2";
						$errMsg = "P2 New Player - game ref parameter missing"; }
					elseif (array_key_exists("PLAYERNAME",$refs) == False) {
						//error - player name parameter missing
						$errCode = "P2N-3";
						$errMsg = "P2 New Player - the player name parameter is missing"; }
					elseif (!preg_match('/^[0-9]+$/',$refs["GAMEREF"])) {
						//game ref must be numeric
						$errCode = "P2N-4";
						$errMsg = "P2 New Player - game ref not numeric"; }
				}
				
				if (strlen($errCode) == 0) {
					//valid input so carry on
					$tbP2NewGameRef = $refs["GAMEREF"]; 
					$tbP2NewPlyrName = $refs["PLAYERNAME"]; 

					//check the password
					$fnResp = fnValidatePlayerNamePW($tbP2NewPlyrName); 
					if (substr($fnResp,0, 3) == 'VP-') {
						$errCode = 'P2N-7';
						$errMsg = $fnResp; }
					else {
						//extract player name
						$inx = strpos($tbP2NewPlyrName,"/",0); 
						$tbP2NewPlyrName = substr($tbP2NewPlyrName,0,$inx); }
				}
				
				if (strlen($errCode) == 0) {
					//all ok so create player 2
					$fnResp = fnProcessNewP2($tbP2NewGameRef, $tbP2NewPlyrName, True);
                    //if (strlen($fnResp) == 0) {
					if (array_key_exists('errCode',$fnResp) == False) {
                        //all ok
                        //call state of play and return state of play message
                        $tbP1NewGameRef = $tbP2NewGameRef;
                        $fnResp = fnStateOfPlay($tbP1NewGameRef);
                   
						if (is_array($fnResp)) {
                            $errCode = "    ";
                            $errMsg = json_encode($fnResp);}
                        else {
                            $errCode = "P2N-9";
                            $errMsg = "P2 New Player - error response from state of play";} }
                    else {
                        //errors occurred setting up player 2
                        $errCode = 'P2N-8,'.$fnResp['errCode'];;
						$errMsg = $fnResp['errMsg'];}
				}
				break;
			
			case "GAME_STATUS":
			case "GAMESTATUS":
				//set type to a standard value
				$refs["TYPE"] = 'GAMES_STATUS';
				$turnCount=0;
				//must have parameters for gameRef; turn count is optional
				if (count($refs) > 3) {
					//error - incorrect number of parameters
					$errCode = "GS-1";
					$errMsg = "Game Status - incorrect number of parameters"; }				
				elseif (array_key_exists("GAMEREF",$refs)==False) {
					//error - game ref parameter missing
					$errCode = "GS-2";
					$errMsg = "Game Status - game ref parameter missing"; }
				elseif (count($refs) == 3) {
					if (array_key_exists("TURNCOUNT",$refs)==False) {
						//error - turn count parameter missing
						$errCode = "GS-4";
						$errMsg = "Game Status - turn count parameter missing"; }
					else {$turnCount = $refs["TURNCOUNT"];}
				}
				if (strlen($errCode) == 0) {
					//valid input so carry on
					$tbP2OldGameRef = $refs["GAMEREF"];
					$fnResp = fnStateOfPlay($tbP2OldGameRef,$turnCount);
					if (is_array($fnResp)) {
						$errCode = "    ";
						$errMsg = json_encode($fnResp);}
					else {
						$errCode = substr($fnResp,0,5);
						$errMsg = substr($fnResp,5,strlen($fnResp)-5);} 
				}
				break;

			case "P1_MOVE":
			case "P1_MOVES":
			case "P1MOVE":
			case "P1MOVES":
			case "P2MOVES":
			case "P2MOVE":
			case "P2_MOVE":
			case "P2_MOVES":
				if (substr($refs['TYPE'],0,2) == 'P1') {$refs['TYPE'] = 'P1_MOVE';}
				if (substr($refs['TYPE'],0,2) == 'P2') {$refs['TYPE'] = 'P2_MOVE';}
				//must have parameters for game ref, player name, move pos and move count. 
				if (count($refs) <> 5) {
					//error - incorrect number of parameters
					$errCode = "PM-1";
					$errMsg = "Player Move - incorrect number of parameters"; }
				elseif (array_key_exists("GAMEREF",$refs) == False) {
					//error - game ref parameter missing
					$errCode = "PM-2";
					$errMsg = "Player Move - game ref parameter missing";}
				elseif (array_key_exists("PLAYERNAME",$refs) == False) {
					//error - player ref parameter missing
					$errCode = "PM-3";
					$errMsg = "Player Move - player name parameter missing"; }
				elseif (array_key_exists("MOVEPOS",$refs) == False) {
					//error - move position parameter missing
					$errCode = "PM-4";
					$errMsg = "Player Move - move position parameter missing"; }
				elseif (array_key_exists("MOVENUM",$refs) == False) {
					//error - move count/num parameter missing
					$errCode = "PM-5";
					$errMsg = "Player Move - move count/num parameter missing"; }

				if (strlen($errCode) == 0) {
					//valid input so carry on
					if (!preg_match('/^[0-9]+$/',$refs["GAMEREF"])) {
						//game ref must be numeric
						$errCode = "PM-7c";
						$errMsg = "Player Move - game ref, move num and move pos must be numeric"; }
					else {
						$tbP1OldGameRef = $refs["GAMEREF"];
						$tbP2OldGameRef = $refs["GAMEREF"]; 
					
						$fnResp = fnValidatePlayerNamePW($refs["PLAYERNAME"]); 
						if (substr($fnResp,0, 3) == 'VP-') {
							$errCode = 'PM-6';
							$errMsg = $fnResp; }
						else {
							$tbPOldPlyrRef = $fnResp; 
		
							if (preg_match('/^[0-9]+$ | ^0\.[1-9]$/',$refs['MOVEPOS']) === false) { 
								//move pos must be numeric or a decimal between 0.1 and 0.9
								$errCode = "PM-7a";
								$errMsg = "Player Move - move pos must be a whole number or decimal between 0.1 and 0.9 - " . $refs['MOVEPOS'] ; }
							else {
								$tbPMovePos = $refs['MOVEPOS']; 
					
								if (preg_match('/^[0-9]+$/',$refs["MOVENUM"]) === false) {
									//move num must be numeric
									$errCode = "PM-7b";
									$errMsg = "Player Move - move num and move pos must be numeric"; }
								else {
									$tbPMoveCount = $refs["MOVENUM"]; }
							}
						}
					}
				}
					
				if (strlen($errCode) == 0) {
					//input has been put into the form. Now run the play function
					$fnResp = fnValidatePlay($tbP2OldGameRef,$tbPOldPlyrRef,$tbPMovePos,$tbPMoveCount); 

					if ($fnResp["errCode"]=="" or $fnResp["errCode"]=="PMO-6" or $fnResp["errCode"]=="PMO-4") {
						//successful move so return state of play
						$fnResp = fnStateOfPlay($tbP2OldGameRef);
						if (is_array($fnResp)) {
							//all ok so send stats of play data back
							$errCode = "    ";
							$errMsg = json_encode($fnResp); }
						else {
							//error from state of play
							$errCode = substr($fnResp,0, 5);
							$errMsg = substr($fnResp,5, strlen($fnResp)- 5); } }
					else {
						//error from fnValidatePlay
						if ($fnResp["errCode"]=="V") {
							$errCode = "PM-8";
							$errMsg = "Player Move - error from validate play"; }
						else {
							$errCode = "PM-8,".$fnResp["errCode"];
							$errMsg = $fnResp["errMsg"]; } }
				}
				break;
					
			case "PLAYER_INFO":
			case "PLAYERINFO":
				//one parameter which is player name 
				if (count($refs) <> 2) {
					//error - incorrect number of parameters
					$errCode = "PI-1";
					$errMsg = "Player info - incorrect number of parameters"; }
				elseif (array_key_exists("PLAYERNAME",$refs) == false) {
					//error - invalid parameter
					$errCode = "PI-2";
					$errMsg = "Player info - requires player name parameter"; }
				
				if (strlen($errCode) == 0) {
					//valid input so carry on
					//search the players table to retrieve player ref
					$playerRow = fnQrySelectPlayer(strtoupper(trim($refs["PLAYERNAME"])));
					if (is_string($playerRow)) {
						$errCode = "PI-5";
						$errMsg = "Player info - player name not found"; }
					else {
						$playerRef = $playerRow['PlayerRef'];
						//prepare output
						$errCode = "    ";
						$errMsg = "player_info(playerName=".strtoupper(trim($refs["PLAYERNAME"]));
						//find any games that involve the player
						$gameRows=fnQrySelectGamesForPlayer($playerRef);
						if (is_string($gameRows)) {
							//output already set up and nothing to add except the closing bracket
							$errMsg .= ")";
							$errMsg = json_encode(array("type" => "player_info", "PlayerName" => strtoupper(trim($refs["PLAYERNAME"])), "Games" => array())); }
						else {
							for ($i=0; $i < count($gameRows); $i++) {
								if ($gameRows[$i]['Winner']==0){$gameState="in progress";}
								elseif ($gameRows[$i]['Winner']==$playerRef){$gameState="won";}
								else {$gameState="lost";}
								$errMsg .= ";Game=".$gameRows[$i]['GameRecID'].",GameState=".$gameState; 
								$games[] = array("GameRef" => $gameRows[$i]['GameRecID'], "AreaSize" => $gameRows[$i]['AreaSize'], "GameState" => $gameState); }
							$errMsg .= ")";
							$errMsg = json_encode(array("type" => "player_info", "PlayerName" => strtoupper(trim($refs["PLAYERNAME"])), "Games"=> $games)) ; }
					}
				}
				break;

			case "OPEN_GAMES":
			case "OPENGAMES":
				//No parameters
				if (count($refs) <> 1) {
					//error - incorrect number of parameters
					$errCode = "OG-1";
					$errMsg = "Open Games - incorrect number of parameters"; }

				if (strlen($errCode) == 0) {
					//valid input so carry on
					//select all games where P2RecID = 0 and join with players table to get player 1 PlayerName
					$openGamesRows = fnQrySelectOpenGames() ;
					if (is_string($openGamesRows)) {
						$errCode = "    ";
						$errMsg = "Open_Games(none)"; 
						$errMsg = json_encode(array("type" => "Open_Games","Games" => array()));}
					else {
						$errCode = "    ";
						$errMsg = "Open_Games(";
						for ($i=0; $i < count($openGamesRows); $i++) {
							$games[] = array("GameRef" => $openGamesRows[$i]['GameRecID'], "Player1Name" => $openGamesRows[$i]['PlayerName']);
							$errMsg = $errMsg."Game=".$openGamesRows[$i]['GameRecID']. "," . $openGamesRows[$i]['PlayerName'] . ";" ; }
						$errMsg = $errMsg.")"	; 
						$errMsg = json_encode(array("type" => "Open_Games","Games" => $games)); }
				}
				break;					

			case "REGISTER":
				//two parameters
				if (count($refs) <> 3) {
					//error - incorrect number of parameters
					$errCode = "RP-1";
					$errMsg = "Register Player - incorrect number of parameters"; }
				elseif (array_key_exists("PLAYERNAME",$refs) == False) {
					//error - player name parameter missing
					$errCode = "RP-2";
					$errMsg = "Register Player - player name parameter missing";}
				elseif (array_key_exists("PIN",$refs) == False) {
					//error - PIN parameter missing
					$errCode = "RP-3";
					$errMsg = "Register Player - PIN parameter missing";}
				else {
					$password = $refs["PIN"];
					$playerName = $refs["PLAYERNAME"];
					
					//PIN cannot be blank
					if (strlen($password)<4 or !ctype_digit($password)) {
						$errCode = "RP-5";
						$errMsg = "Register Player - PIN must be 4 or more digits";}
					else {
						//player name cannot be blank
						if (strlen($playerName)==0) {
							$errCode = "RP-6";
							$errMsg = "Register Player - player name cannot be blank";}
						//player name comprise letters and numbers only
						elseif (!preg_match('/^[a-zA-Z0-9 ]+$/',$playerName)) {
							$errCode = "RP-6";
							$errMsg = "Register Player - player name - " . $playerName . " - must contain only letters and numbers";}
						else {
							//make sure player name doesn't exist already
							$fnResp = fnQrySelectPlayer($playerName);
							if ($fnResp != "player not found") {
								$errCode = "RP-6";
								$errMsg = "Register Player - player name already exists";}
						}
					}
				}
					
				if (strlen($errCode) == 0) {
					//valid input so carry on
					//get a new player ref
					$refsRow = fnReferencesTable("new player ref");
					if (is_string($refsRow)) {
						$errCode = "RP-8";
						$errMsg= "Error setting up new player ref"; }
					else {
						$playerRef = $refsRow['NextPlayerRef']; }
					//create new player record
					$fnResp = fnQryAddPlayer($playerRef,$playerName,$password,$hint);
					if (strlen($fnResp) > 0) {
						$errCode = "RP-7";
						$errMsg = "Register Player - error in fnQryAddPlayer [$fnResp]";}
					else {
						$errCode = "    ";
						$errMsg = "register(playername=".$playerName.";PIN=".$password;
						$errMsg = json_encode(array("type" => "register","playername" => $playerName, "PIN" => $password)); }
				}
				break;
				
			case "RESIGN":
				//two parameters
				if (count($refs) <> 3) {
					//error - incorrect number of parameters
					$errCode = "PR-1";
					$errMsg = "Player Resigns - incorrect number of parameters"; }
				elseif (array_key_exists("PLAYERNAME",$refs) == False) {
					//error - player name parameter missing
					$errCode = "PR-2";
					$errMsg = "Player Resigns - player name parameter missing";}
				elseif (array_key_exists("GAMEREF",$refs) == False) {
					//error - game ref parameter missing
					$errCode = "PR-3";
					$errMsg = "Player Resigns - game ref parameter missing";}

				if (strlen($errCode) == 0) {
					//valid input so carry on
					$gameRef = $refs["GAMEREF"];
					$playerName = $refs["PLAYERNAME"];
					//check player name/PIN
					$fnResp = fnValidatePlayerNamePW($playerName); 
					//if (!ctype_digit($fnResp)) {
					if (substr($fnResp,0, 3) == 'VP-') {
						$errCode = "PR-7";
						$errMsg = $fnResp; }
					else {
						//Update game record
						$playerRef = $fnResp;
						$gameRow = fnQrySelectGame($gameRef);
						if (is_string($gameRow)) {
							$errCode = "PR-4";
							$errMsg = "Player Resigns - error retrieving game record";}
						else {
							//check player is part of game
							if ($playerRef != $gameRow['P1RecID'] And $playerRef != $gameRow['P2RecID']) {
								$errCode = "PR-5";
								$errMsg = "Player Resigns - player is not part of game";
								fnLogMessageToDb($errCode.$errMsg."/".$playerRef."/".$gameRow['P1RecID']."/".$gameRow['P2RecID']);}
							//check game hasn't already finished
							elseif ($gameRow['Winner'] != 0) {
								$errCode = "PR-6";
								$errMsg = "Player Resigns - game is already finished";
								fnLogMessageToDb($errCode.$errMsg."/".$gameRow['Winner']);}
							elseif ($gameRow['TurnCount'] >= 10) {
								//set a winner and save game
								if ($playerRef == $gameRow['P1RecID']) {
									$gameRow['Winner'] = $gameRow['P2RecID'] ;}
								else {
									$gameRow['Winner'] = $gameRow['P1RecID'] ;}
								$fnResp = fnQryUpdateGame($gameRow['GameRecID'],$gameRow['P1RecID'],$gameRow['P2RecID'],$gameRow['LastPlayerID'],$gameRow['StartDate'],$gameRow['LastUpdated'],$gameRow['AreaSize'],$gameRow['Winner'],$gameRow['TurnCount']);
								if (strlen($fnResp) > 0) {
									$errCode = "PR-8";
									$errMsg = "Player resigns - ". $fnResp;	}
								else {
									$errCode = "    ";}
							}
							else {
								//set game to void and save game
								$gameRow['Winner'] = -1*$playerRef ;
								$fnResp = fnQryUpdateGame($gameRow['GameRecID'],$gameRow['P1RecID'],$gameRow['P2RecID'],$gameRow['LastPlayerID'],$gameRow['StartDate'],$gameRow['LastUpdated'],$gameRow['AreaSize'],$gameRow['Winner'],$gameRow['TurnCount']);
								if (strlen($fnResp) > 0) {
									$errCode = "PR-9";
									$errMsg = "Player resigns - ". $fnResp;	}
								else {
									$errCode = "    "; }
							}
							//if there are no errors then return the state of play message
							if ($errCode == "    ") {
								$fnResp = fnStateOfPlay($gameRef);
								if (is_array($fnResp)) {
									//all ok so send stats of play data back
									$errCode = "    ";
									$errMsg = json_encode($fnResp); }
								else {
									//error from state of play
									$errCode = substr($fnResp,0, 5);
									$errMsg = substr($fnResp,5, strlen($fnResp)- 5); } 
							}
						}
					}
				}
				break;
				
			case "GO_BACK":
			case "GOBACK":
				//revert to an earlier stage in the game. Only GAMECENTRAL can use this
				//three parameters - playername, game ref and turncount
				fnLogMessageToDb("Go back message");
				if (count($refs) <> 4) {
					//error - incorrect number of parameters
					$errCode = "GB-1";
					$errMsg = "Go back - incorrect number of parameters"; }
				elseif (array_key_exists("PLAYERNAME",$refs) == False) {
					//error - player name parameter missing
					$errCode = "GB-2";
					$errMsg = "Go back - player name parameter missing";}
				elseif (array_key_exists("GAMEREF",$refs) == False) {
					//error - game ref parameter missing
					$errCode = "GB-3";
					$errMsg = "Go back - game ref parameter missing";}
				elseif (array_key_exists("TURNCOUNT",$refs) == False) {
					//error - turn count parameter missing
					$errCode = "GB-4";
					$errMsg = "Go back - turn count parameter missing";}
				else {
					//get game record
					$gameRef = $refs["GAMEREF"];
					$playerName = $refs["PLAYERNAME"];
					$turnCountGB = $refs["TURNCOUNT"];
					$gameRow = fnQrySelectGame($gameRef);
					if (is_string($gameRow)) {
						$errCode = "GB-6";
						$errMsg = "Go back - error retrieving game record";}
					elseif ($turnCountGB < 1 OR $turnCountGB >= $gameRow['TurnCount']) {
						$errCode = "GB-7";
						$errMsg = "Go back - turn count out of range";}
				}
				fnLogMessageToDb("passed go message checks");
				if (strlen($errCode) == 0) {
					//valid input so carry on
					//check player name/PIN is valid and that it is GAMECENTRAL
					$fnResp = fnValidatePlayerNamePW($playerName); 
					if (substr($fnResp,0, 3) == 'VP-' OR $fnResp != 1) {
						$errCode = "GB-5";
						$errMsg = $fnResp; }
					else {
						/*
						Overwrite turn count 0 records with data from corresponding record for GOBACK turn count then delete every record with a turn count greater than GOBACK
						*/
						//begin database transaction
						fnLogMessageToDb("start transaction");
						$conn->beginTransaction();
						$logMsgs=array("rewrite log msgs after rollback");
						//check for any errors
						if (strlen($errCode) == 0) {
							//all ok so do the updates 
							//overwrite Game record for turnCount0 with values from record for turnCount Go BACK
							fnLogMessageToDb("Get game record for turn coutn = GOBACK");
							array_push($logMsgs,"Get game record for turn count = GOBACK");
							$gameRowGB = fnQrySelectGame($gameRef, $turnCountGB); 
							if (is_string($gameRowGB)) {
								$errMsg = "Game " . $gameRef . " and turncount ". $turnCountGB . " not found";
								$errCode = "GB-8"; 
								fnLogMessageToDb($errCode.$errMsg); 
								array_push($logMsgs,$errCode.$errMsg); }
							else {
								fnLogMessageToDb("update game record for turn count = 0  with value from turn count = GOBACK");
								array_push($logMsgs,"update game record for turn count = 0  with value from turn count = GOBACK");
								$fnResp = fnQryUpdateGame($gameRowGB['GameRecID'],$gameRowGB['P1RecID'],$gameRowGB['P2RecID'],$gameRowGB['LastPlayer'],$gameRowGB['StartDate'],date("Y-m-d H:i:s"),$gameRowGB['AreaSize'],$gameRowGB['Winner'],$gameRowGB['TurnCount'],0);
								//fnQryUpdateGame ($gameRef, $p1Ref, $p2Ref, $lastPlayer, $startDate, $lastUpdated, $areaSize, $winner, $turnCount, $turnCountVer)
								if (strlen($fnResp) > 0) {
									$errCode = "GB-9";
									$errMsg = "Go Back - " . $fnResp; 
									fnLogMessageToDb($errCode.$errMsg); 
									array_push($logMsgs,$errCode.$errMsg); }
								else {
									//delete Game records with turn count > Go Back
									fnLogMessageToDb("delete game records with turn count > GOBACK");
									array_push($logMsgs,"delete game records with turn count > GOBACK"); 
									$fnResp = fnQryDeleteGames($gameRef, $turnCountGB); 
									if (strlen($fnResp) > 0) {
										$errCode = "GB-I";
										$errMsg = "Go Back - " . $fnResp; 
										fnLogMessageToDb($errCode.$errMsg); 
										array_push($logMsgs,$errCode.$errMsg); }
									else {
										//delete the Stacks records with turn count > Go BACK
										fnLogMessageToDb("delete stacks with turn count > GOBACK");
										array_push($logMsgs,"delete stacks with turn count > GOBACK");
										$fnResp = fnQryDeleteStacksForManyTurns($gameRef, $turnCountGB);
										if (strlen($fnResp) > 0) {
											$errCode = "GB-A";
											$errMsg = "Go Back - " . $fnResp; 
											fnLogMessageToDb($errCode.$errMsg); 
											array_push($logMsgs,$errCode.$errMsg); }
										else {
											//get the Stacks records with turn count = 0
											fnLogMessageToDb("get stacks with turn count = 0");
											array_push($logMsgs,"get stacks with turn count = 0");
											$stackRows0 = fnQrySelectStacks($gameRef, 0);
											if (is_array($stackRows0)==FALSE) {
												$errCode = "GB-B";
												$errMsg = "Stacks for game " . $gameRef . " and turncount 0 not found"; 
												fnLogMessageToDb($errCode.$errMsg); 
												array_push($logMsgs,$errCode.$errMsg); }
											else {
												//get the Stacks records with turn count = GOBACK
												fnLogMessageToDb("update stacks with turn count = GOBACK");
												array_push($logMsgs,"update stacks with turn count = GOBACK");
												$stackRowsGB = fnQrySelectStacks($gameRef,$turnCountGB);
												if (is_array($stackRowsGB)==FALSE) {
													$errMsg = "Stacks for game " . $gameRef . " and turncount " . $turnCountGB . " not found";
													$errCode = "GB-C"; 
													fnLogMessageToDb($errCode.$errMsg); 
													array_push($logMsgs,$errCode.$errMsg); }
												else {
													//copy the Stacks records with turn count = GO BACK into stacks with turn count = 0
													fnLogMessageToDb("copy the stacks with turn count = GO BACK into the stacks with turn count = 0");
													array_push($logMsgs,"copy the stacks with turn count = GO BACK into the stacks with turn count = 0");
													$maxI = count($stackRowsGB);
													for ($i=0; $i<$maxI; $i++) {
														$fnResp = fnQryUpdateStack($stackRowsGB[$i]['StackNum'],$stackRowsGB[$i]['StackHeight'],$stackRowsGB[$i]['OwnerPNum'],$stackRows0[$i]['StackRecID'],0);
														//fnQryUpdateStack($stackNum, $stackHeight, $top, $stackRecID, $turnCount)
														if (strlen($fnResp) > 0 ){
															$errCode = "GB-D";
															$errMsg = "Go Back - " . $fnResp; 
															fnLogMessageToDb($errCode.$errMsg); 
															array_push($logMsgs,$errCode.$errMsg); 
															break; }
													}
													//if there were no errors then carry on
													if (strlen($errCode) == 0) {
														//delete the counters records with turn count > Go BACK
														fnLogMessageToDb("delete counters with turn count > GO BACK");
														array_push($logMsgs,"delete counters with turn count > GO BACK"); 
														$fnResp = fnQryDeleteCountersForManyTurns($gameRef, $turnCountGB);
														if (strlen($fnResp) > 0) {
															$errCode = "GB-E";
															$errMsg = "Go Back - " . $fnResp; 
															fnLogMessageToDb($errCode.$errMsg); 
															array_push($logMsgs,$errCode.$errMsg); }
														else {
															//delete the counters records with turn count = 0
															fnLogMessageToDb("delete counters with turn count = 0");
															array_push($logMsgs,"delete counters with turn count = 0");
															$fnResp = fnQryDeleteCountersForOneTurn($gameRef, 0);
															if (strlen($fnResp) > 0) {
																$errCode = "GB-F";
																$errMsg = "Go Back - " . $fnResp; 
																fnLogMessageToDb($errCode.$errMsg); 
																array_push($logMsgs,$errCode.$errMsg); }
															else {
																//copy the Counters records with turn count = GO BACK into new Counters records with turn count = 0
																fnLogMessageToDb("select counters");
																$countersRowsGB = fnQrySelectAllCounters($gameRef,$turnCountGB);
																if (is_string($countersRowsGB)) {
																	$errMsg = "Counter records for game " . $gameRef . " and turncount " . $turnCountGB . "not found";
																	$errCode = "GB-G"; 
																	fnLogMessageToDb($errCode.$errMsg); 
																	array_push($logMsgs,$errCode.$errMsg); }
																else {
																	fnLogMessageToDb("copy counters");
																	array_push($logMsgs,"copy counters"); 
																	for ($i=0; $i<count($countersRowsGB); $i++) {
																		$fnResp = fnQryAddCounter($countersRowsGB[$i]['GameRecID'],$countersRowsGB[$i]['StackNum'],$countersRowsGB[$i]['PlayerRef'],$countersRowsGB[$i]['CounterHeight'], 0); 
																		if (strlen($fnResp) > 0 ){
																			$errCode = "GB-H";
																			$errMsg = "Go Back - " . $fnResp; 
																			fnLogMessageToDb($errCode.$errMsg); 
																			array_push($logMsgs,$errCode.$errMsg); 																			
																			break; }
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}	 
						}
						if (strlen($errCode) > 0) { 
							//an error has occurred rollback any database uopdates
							$conn->rollback(); 
							//(re)write log messages
							foreach ($logMsgs as $msgs) {fnLogMessageToDb($msgs);}
							fnLogMessageToDb ("Go Back - Rollback"); }
						else {
							$conn->commit(); 
							$fnResp = fnStateOfPlay($gameRef);
							if (is_array($fnResp)) {
								//all ok so send stats of play data back
								$errCode = "";
								$errMsg = json_encode($fnResp); }
							else {
								//error from state of play
								$errCode = substr($fnResp,0, 5);
								$errMsg = substr($fnResp,5, strlen($fnResp)- 5); }
						}
					}
				}
				break;
	
			default:
				//error - invalid input
				$errCode = "PrI-2";
				$errMsg = "input type " . $refs["TYPE"] . " not recognised";
		}
	}

	fnLogMessageToDb("(fnProcessInput) status after case statements: " . $errCode.$errMsg);
	if (strlen(trim($errCode))==0){
		return $errMsg;}
	else{
		return json_encode(array("type"=>"error","ErrorCode"=>$errCode,"ErrorMsg"=>$errMsg));}
} 
//End fnProcessInput
function fnFindParams($callString) {
 	fnLogMessageToDb("start fnFindParams");
	
	//decode the json and create an assoicative array
	$refs = json_decode($callString, True);
	if (is_null($refs)) {
		$refs['errCode'] = 'FP-1' ; 
		$jsErrMsg = json_last_error() ;
		$refs['errMsg'] = 'Find Params - invalid json - '. $jsErrMsg ; 
		fnLogMessageToDb($refs['errCode'].$refs['errMsg']);
		return $refs; }
	else {
		return $refs;}
}
//End fnFindParams
function fnProcessNewP1($tbP1NewPlyrName, $silent, $password="", $hint=""){
 	fnLogMessageToDb("start fnProcessNewP1");
	//globals
	global $tbP1NewGameRef, $tbP1NewPlyrRef;
	global $tbP2NewGameRef, $tbP2NewPlyrRef, $logLevel; 

	$errMsg = ""; $errCode = "";
	$gameRef=0;
	$playerRef=0;
	$playerRow=null; $refsRow=null;
	
	//if player name is not blank then see if it already exists
	if (strlen($tbP1NewPlyrName) > 0) {
		$playerRow = fnQrySelectPlayer(strtoupper(trim($tbP1NewPlyrName)));
		if (is_array($playerRow)) {
			$playerRef = $playerRow['PlayerRef'];}
	}

	//get the next game and player refs
	if ($playerRef > 0) {
		//already have a player ref so just get new game ref
		$refsRow = fnReferencesTable("new game ref");
		if (is_array($refsRow)) {
			$gameRef = $refsRow['NextGameRef'];	}
		else {
			$errMsg = 'New player 1 - error setting up new game reference';
			$errCode = 'NP1-1';
			fnLogMessageToDb($errCode.$errMsg);
			return array('errCode' => $errCode, 'errMsg' => $errMsg); }
	}
	else {
		$refsRow = fnReferencesTable('new both');
		if  (is_array($refsRow)) {
			$gameRef = $refsRow['NextGameRef'];
			$playerRef = $refsRow['NextPlayerRef']; }
		else {
			$errMsg = 'New player 1 - error setting up new game and player references';
			$errCode = 'NP1-2';
			fnLogMessageToDb($errCode.$errMsg);
			return array('errCode' => $errCode, 'errMsg' => $errMsg); }
	}

	//if player name is not blank and its not already on record then associate input name with new player 
	if (strlen($tbP1NewPlyrName) > 0 and is_array($playerRow)==FALSE) {
		$fnResp = fnQryAddPlayer($playerRef, strtoupper(trim($tbP1NewPlyrName)),$password,$hint); 
		if (strlen($fnResp) > 0) {
			$errCode = 'NP1-3';
			$errMsg = $fnResp ;
			fnLogMessageToDb($errCode.$errMsg);
			return array('errCode' => $errCode, 'errMsg' => $errMsg); }
	}

	//create a new game records
	$areaSize=8;
	$fnResp = fnSetUpGameRecs($gameRef, $playerRef, $areaSize);
	if (strlen($fnResp)>0) {
		$errCode = 'NP1-4';
		$errMsg = $fnResp; 
		fnLogMessageToDb($errCode.$errMsg);
		return array('errCode' => $errCode, 'errMsg' => $errMsg);}
	
	//return the game ref which means all OK
	return array('gameRef' => $gameRef);
}
//End fnProcessNewP1
function fnSetUpGameRecs($gameRef, $playerRef, $areaSize) {
	global $conn, $logLevel;
 	fnLogMessageToDb("start fnSetUpGameRecs");
	
	//begin database transaction
	$errCode = ""; $errMsg = "";
	$conn->beginTransaction();

	//insert new record into the Games table
	$fnResp = fnQryAddGame($gameRef, $playerRef, 0, 0, date("Y-m-d H:i:s"),date("Y-m-d H:i:s"), $areaSize, 0, 0, 0);
	if (strlen($fnResp)>0) {
		$errCode = "SGR-2";
		$errMsg = "Set up Game Recs - " . $fnResp; 
		fnLogMessageToDb($errCode.$errMsg); }	
	
	//insert new records into the Stacks table
    for ($gStep = 0; $gStep <= $areaSize + 1; $gStep++) {
		if (strlen($errCode)>0) { break; }
		if ($gStep == 0) {
			//set up the player 1 reserve 
			$fnResp = fnQryAddStack($gameRef, $gStep, $areaSize+2, $playerRef, 0); 
			if (strlen($fnResp)>0) {
				$errCode = "SGR-1";
				$errMsg = "Set up Game Recs - " . $fnResp; 
				fnLogMessageToDb($errCode.$errMsg); }
		}
		else {
			$fnResp = fnQryAddStack($gameRef, $gStep, 0, 0, 0); 
			if (strlen($fnResp)>0) {
				$errCode = "SGR-4";
				$errMsg = "Set up Game Recs - " . $fnResp;
				fnLogMessageToDb($errCode.$errMsg);	}
		}
		if (strlen($errCode)>0) { break; }

		//if gStep = 0 the it is the reserve for player 1 so set up counter (layer) records
		if ($gStep == 0) {
			for ($counterHeight = 1; $counterHeight <= $areaSize+2; $counterHeight++) {
				$fnResp = fnQryAddCounter($gameRef, $gStep, $playerRef, $counterHeight, 0); 
				if (strlen($fnResp)>0) {
					$errCode = "SGR-3";
					$errMsg = "Set up Game Recs - " . $fnResp;
					fnLogMessageToDb($errCode.$errMsg);	}
				if (strlen($errCode)>0) { break 2; }
			}
		}
	}
	//end database transaction
	if (strlen($errCode)>0) { 
		//if an error has occurred rollback any database uopdates
		$conn->rollback(); }
	else {
		$conn->commit(); }
	return $errCode . $errMsg;
}
//End fnSetUpGameRecs
function fnProcessNewP2($tbP2NewGameRef, $tbP2NewPlyrName, $silent, $password = "", $hint = ""){
 	fnLogMessageToDb("start fnProcessNewP2");
	
	global $tbP1MoveCount, $tbP1MovePos, $tbP2MoveCount, $tbP2MovePos, $logLevel ;
	
	$playerRef = 0;
	$fnResp =0;
	$errMsg = "";
	$refsRow = null; $playerRow = null;

	//if player name is not blank then see if it already exists
	if (strlen($tbP2NewPlyrName) > 0) {
		// . "new player name: " . $tbP2NewPlyrName .".";
		$playerRow = fnQrySelectPlayer(strtoupper(trim($tbP2NewPlyrName)));
		if (is_array($playerRow)) {
		$playerRef = $playerRow['PlayerRef']; } }

	if ($playerRef == 0) {
		//get a new player ref
		$refsRow = fnReferencesTable("new player ref");
		if  (is_string($refsRow)) {
			$errCode = "NP2-1";
			$errMsg = "New player 2 - error setting up new player references";
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode, "errMsg"=>$errMsg); }
		else {
			$playerRef = $refsRow['NextPlayerRef']; } }

	//if player name is not blank and its not already on record then associate input name with new player 
	if (strlen($tbP2NewPlyrName) > 0 And is_string($playerRow)) {
		//adding new player
		$fnResp = fnQryAddPlayer($playerRef, strtoupper(trim($tbP2NewPlyrName)),$password,$hint); 
		if (strlen($fnResp) > 0) {
			$errCode = "NP2-6";
			$errMsg = "New player 2 - " . $fnResp ;
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode, "errMsg"=>$errMsg); }
	}

	//display player ref
	$tbP2NewPlyrRef = $playerRef;

	//'set up player 2 reserve in database
	$fnResp = fnSetUpPlayer2($playerRef, $tbP2NewGameRef, $password, $hint);
        if ($fnResp == 1) {
			$errMsg = "New player 2 - invalid game ref";
			$errCode = "NP2-2";
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode, "errMsg"=>$errMsg); }
		elseif ($fnResp == 2) {
			$errMsg = "New player 2 - player 2 already exists";
			$errCode = "NP2-4";
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode, "errMsg"=>$errMsg); }
		elseif ($fnResp == 3) {
			$errMsg = "New player 2 - database updates failed";
			$errCode = "NP2-7";
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode, "errMsg"=>$errMsg); }
		elseif ($fnResp > 0) {
			$errMsg = "New player 2 - unknown error";
			$errCode = "NP2-5";
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode, "errMsg"=>$errMsg); }

	//clear any player input 
	$tbP1MoveCount = ""; $tbP1MovePos = ""; $tbP2MoveCount = ""; $tbP2MovePos = "";

	//return empty errMsg which means all OK
	return $errMsg;
}
//End fnProcessNewP2
function fnSetUpPlayer2($playerRef, $tbP2NewGameRef, $password, $hint) {
 	fnLogMessageToDb("start fnSetUpPlayer2");
	global $conn, $logLevel;
	
	$areaSize =0;

	//find game record
	$gameRow = fnQrySelectGame($tbP2NewGameRef);
	if (is_string($gameRow)) {
		return 1; }

	//the player 2 ref should be zero
	if ($gameRow['P2RecID'] <> 0) {
		return 2;}

	//begin database transaction
	$errCode = ""; $errMsg = "";
	$conn->beginTransaction();

	$fnResp = fnQryUpdateGame ($tbP2NewGameRef,$gameRow['P1RecID'],$playerRef,$playerRef,$gameRow['StartDate'],date("Y-m-d H:i:s"),$gameRow['AreaSize'],$gameRow['Winner'],$gameRow['TurnCount'] );
	if (strlen($fnResp) > 0) {
		$errCode = "SP2-1";
		$errMsg = "Setup player 2 - " . $fnResp ;
		fnLogMessageToDb($errCode.$errMsg); }

	$areaSize = $gameRow['AreaSize'];

	//find the GameStep records for this the player 2 reserve 
	$stackRows = fnQrySelectStacks($tbP2NewGameRef,0);

	//set height of reserve and save record
	$fnResp = fnQryUpdateStack($areaSize+1, $areaSize+2, $playerRef,$stackRows[$areaSize+1]['StackRecID']);
	if (strlen($fnResp) > 0) {
		$errCode = "SP2-2";
		$errMsg = "Setup player 2 - " . $fnResp ;
		fnLogMessageToDb($errCode.$errMsg); }

	//set up layers records for player 2 reserve
	for ($counterHeight=1; $counterHeight <= $areaSize + 2; $counterHeight++) {
		$fnResp = fnQryAddCounter($tbP2NewGameRef, $areaSize+1, $playerRef, $counterHeight, 0); 
		if (strlen($fnResp) > 0) {
			$errCode = "SP2-3";
			$errMsg = "Setup player 2 - " . $fnResp ;
			fnLogMessageToDb($errCode.$errMsg); 
			break; }
	}

	//end database transaction
	if (strlen($errCode)>0) { 
		//if an error has occurred rollback any database uopdates
		$conn->rollback(); 
		return 3; }
	else {
		$conn->commit(); }
		
	return 0;
}
//End fnSetUpPlayer2
function fnStateOfPlay($gameKey, $turnCount=0) {
 	fnLogMessageToDb("start fnStateOfPlay for ".$gameKey);
	global $tbP1OldGameRef, $tbP1NewGameRef;
	global $tbP2OldGameRef, $logLevel;
	$errMsg = ""; $errCode = ""; $sopData = "";
	$gameRef = ""; $hashKey = "";

	//get game record
	if (strlen($gameKey)<20) {
		fnLogMessageToDb("get game record using game ref");
		$gameRef = $gameKey;
		$gameRow = fnQrySelectGame($gameRef, $turnCount);
		if (is_array($gameRow)==FALSE) {
			$errCode = "SoP-1";
			$errMsg = "Game " . $gameRef . " and turn count " . $turnCount . " not found in fnQrySelectGame";
			fnLogMessageToDb($errCode.$errMsg); }
	}
	else {
		fnLogMessageToDb("get game record using hashKey");
		$hashKey = $gameKey;
		$gameRow = fnQrySelectNmxGame($hashKey);
		if (is_string($gameRow)) {
			$errCode = "SoP-4";
			$errMsg = $gameRow;	
			fnLogMessageToDb ($errCode.$errMsg); }
		else {
			$gameRow['P1Name'] = "One";
			$gameRow['P2Name'] = "Two"; 
			if ($gameRow['P1RecID'] == $gameRow['LastPlayerID']) {$gameRow['LPName'] = $gameRow['P2Name'];} else {$gameRow['LPName'] = $gameRow['P1Name'];} }
	}
	if (strlen($errCode)==0) {
		//calculate dimensions
		$sideSpacer = 8;
		$areaSize = $gameRow['AreaSize'];

		//start building output list
		//$sopData = "gameRef=" . $gameRef . ";player1Name=" . $gameRow['P1Name'] . ";player2Name=" . $gameRow['P2Name'] ;
		$sopArray = array("type" => "game_status", "GameRef" => $gameRow['GameRecID'], "Player1Name" => $gameRow['P1Name'], "Player2Name" => $gameRow['P2Name']) ;
		//$sopData = $sopData . ";turnCount=" . $gameRow['TurnCount'] . ";lastPlayer=" . $gameRow['LPName'];
		$sopArray += array("TurnCount" => $gameRow['TurnCount']);
		$sopArray += array("LastPlayer" => $gameRow['LPName']);
		//$sopData = $sopData . ";areaSize=" . $gameRow['AreaSize'] . ";winner=" ;
		$sopArray += array("AreaSize" => $gameRow['AreaSize']);
		if ($gameRow['Winner'] == $gameRow['P1RecID']) {
			//$sopData = $sopData . $gameRow['P1Name'];
			$sopArray += array("Winner" => $gameRow['P1Name']); } 
		elseif ($gameRow['Winner'] == $gameRow['P2RecID']){ 
			//$sopData = $sopData . $gameRow['P2Name'];
			$sopArray += array("Winner" => $gameRow['P2Name']);} 
		elseif ($gameRow['Winner'] < 0){ 
			//$sopData = $sopData . "Void";
			$sopArray += array("Winner" => "Void");} 
		else { 
			$sopArray += array("Winner" => "");} 

		//get pstacks record
		if ($hashKey == "") {
			$stackRows = fnQrySelectStacks($gameRef,$turnCount);
			if (is_array($stackRows)==FALSE) {
				$errCode = "SoP-2";
				$errMsg = $stackRows;
				fnLogMessageToDb($errCode.$errMsg); 
			}
		}
		else {
			$stackRows = fnQrySelectNmxStacks($hashKey);
			if (is_string($stackRows)) {
				$errMsg = $stackRows;
				$errCode = "SoP-3";
				fnLogMessageToDb($errCode.$errMsg); }
		}
	}
	if (strlen($errCode)==0) {
		//process playing area
		$sopArray["Stacks"] = array();
		for ($stepNum=1; $stepNum <= $areaSize+2; $stepNum++) {
			//$sopData = $sopData . ';stack=' . $stepNum . ',' . $stackRows[$stepNum]['StackHeight'] . ',';
			$sopArray["Stacks"][$stepNum-1] = array("Stack" => $stepNum-1, "Height" => $stackRows[$stepNum-1]['StackHeight']);
			if ($stackRows[$stepNum-1]['OwnerPNum'] == $gameRow['P1RecID'] And $gameRow['P1RecID'] <> 0) {
				//$sopData = $sopData . "P1";
				$sopArray["Stacks"][$stepNum-1] += array("Top" => "1"); } 
			elseif ($stackRows[$stepNum-1]['OwnerPNum'] == $gameRow['P2RecID'] And $gameRow['P2RecID'] <> 0) {
				//$sopData = $sopData . "P2";
				$sopArray["Stacks"][$stepNum-1] += array("Top" => "2"); } 
			else {
				//$sopData = $sopData . "0";
				$sopArray["Stacks"][$stepNum-1] += array("Top" => ""); }
		}
	}
	//if there is an error message then return it otherwise return the state of play data 
	if (strlen($errCode) > 0) {
		fnLogMessageToDb($errCode.$errMsg);
		return $errCode.$errMsg;}
	else {
		return $sopArray;}
}
//End fnStateOfPlay
function fnDataChecks($gameKey) {
	fnLogMessageToDb("starting data checks for gameKey ".$gameKey);
	global $logLevel;
	//get game record
	if (strlen($gameKey)<20) {
		$gameRecID=$gameKey; }
	else {
		$gameRecID = 0;
		$hashKey = $gameKey; }

	$errCode = "";
	//get the relevant game, stacks and counters records
	if ($gameRecID>0) {
		//get game record using GameRecID
		fnLogMessageToDb("get game record using GameRecID");
		$gameRow = fnQrySelectGameByID($gameRecID);
		if (is_string($gameRow)) {
			$errCode = "DCH-1";
			$errMsg = $gameRow; 
			fnLogMessageToDb ($errCode.$errMsg); }
		else {
			fnLogMessageToDb("get counters records using GameRef and TurnCount");
			$countersRows = fnQrySelectAllCounters($gameRow['GameRecID'], $gameRow['TurnCountVer']);
			if (is_string($countersRows)) {
				$errCode = "DCH-3";
				$errMsg = $countersRows;
				fnLogMessageToDb ($errCode.$errMsg); }
			else {
				//get stacks records
				fnLogMessageToDb("get stacks records using GameRef and TurnCount");
				$stackRows = fnQrySelectStacks($gameRow['GameRecID'], $gameRow['TurnCountVer']);
				if (is_string($stackRows)) {
					$errCode = "DCH-2";
					$errMsg = $stackRows; 
					fnLogMessageToDb ($errCode.$errMsg); }
			}
		}
	}
	else {
		//get the relevant game, stacks and counters records using hash key
		fnLogMessageToDb("get game record using hashKey");
		$gameRow = fnQrySelectNmxGame($hashKey);
		if (is_string($gameRow)) {
			$errCode = "DCH-4";
			$errMsg = $gameRow;	
			fnLogMessageToDb ($errCode.$errMsg); }
		else {
			//get counters records
			fnLogMessageToDb("get counters records using hashKey");
			$countersRows = fnQrySelectAllNmxCounters($hashKey);
			if (is_string($countersRows)) {
				$errCode = "DCH-6";
				$errMsg = $countersRows; 
				fnLogMessageToDb ($errCode.$errMsg); }		
			else {
				//get stacks records
				fnLogMessageToDb("get stacks records using hashKey");
				$stackRows = fnQrySelectNmxStacks($hashKey);
				if (is_string($stackRows)) {
					$errCode = "DCH-5";
					$errMsg = $stackRows;
					fnLogMessageToDb ($errCode.$errMsg); }
			}
		}
	}
	//check for errors
	if (strlen($errCode)>0) {
		echo $errCode.$errMsg;
		return $errCode.$errMsg; }

	//check game record
	echo "checking game " . $gameRow['GameRecID'] . "v" . $gameRow['TurnCountVer'] . " (key=" . $gameKey . ")" . "<br>";
	//Must be areaSize + 2 stack records with the same gameKey
	if (count($stackRows) != $gameRow['AreaSize'] + 2) {
		$errCode = "DCH-7";
		$errMsg = "Wrong number of stacks " . count($stackRows); 
		echo $errCode.$errMsg."<br>";
		fnLogMessageToDb($errCode.$errMsg);}
	//Must be (areaSize + 2) * 2 counters records with the same gameKey
	if (count($countersRows) != ($gameRow['AreaSize'] + 2)*2) {
		$errCode = "DCH-8";
		$errMsg = "Wrong number of counters " . count($countersRows); 
		echo $errCode.$errMsg."<br>";
		fnLogMessageToDb($errCode.$errMsg);}

	//check stack records
	//The Top for each stack must be zero or it must match one of the player refs in the Game record 
	$maxI = count($stackRows);
	for ($i=0; $i<$maxI; $i++) {
		if ($stackRows[$i]['OwnerPNum'] != $gameRow['P1RecID'] and $stackRows[$i]['OwnerPNum'] != $gameRow['P2RecID'] and $stackRows[$i]['OwnerPNum'] != 0) {
			$errCode = "DCH-9";
			$errMsg = "Invalid stack. Top = " . $stackRows[$i]['OwnerPNum'] . " for stack " . $stackRows[$i]['StackRecID'];
			echo $errCode.$errMsg."<br>";
			fnLogMessageToDb($errCode.$errMsg); }
		//if height is zero the top must be zero
		if ($stackRows[$i]['StackHeight'] == 0 and $stackRows[$i]['OwnerPNum'] != 0) {
			$errCode = "DCH-G";
			$errMsg = "Stack height is zero but Top is not zero on stack " . $stackRows[$i]['StackRecID'];
			echo $errCode.$errMsg."<br>";
			fnLogMessageToDb($errCode.$errMsg); }
		else {
			//If the Top is not zero then there must be a counter for the same stack and the same height
			$found = FALSE;
			if ($stackRows[$i]['OwnerPNum'] != 0) {
				$maxJ = count($countersRows);
				for ($j=0; $j<$maxJ; $j++){
					if ($countersRows[$j]['StackNum'] == $i and $stackRows[$i]['OwnerPNum'] == $countersRows[$j]['PlayerRef'] and $stackRows[$i]['StackHeight'] == $countersRows[$j]['CounterHeight']) {
						$found = TRUE;
						break; }
				}
				if ($found == FALSE) {
					$errCode = "DCH-E";
					$errMsg = "No top counter for stack " . $stackRows[$i]['StackRecID'];
					echo $errCode.$errMsg."<br>";
					fnLogMessageToDb($errCode.$errMsg); }
			}
		}
	}
	
	//check counters records
	//The counter records for a particular stack must be sequential counterHeight starting at 1
	$lastStackNum = -1;
	$maxI = count($countersRows);
	for ($i=0; $i<$maxI; $i++) {
		//if stack num changes then counterHeight must be 1 otherwise counterHeight must be 1 greater than the previous one
		if ($countersRows[$i]['StackNum'] != $lastStackNum) {
			if ($countersRows[$i]['CounterHeight'] != 1) {
				$errCode = "DCH-A";
				$errMsg = "counter height not equal to 1 for counter " . $countersRows[$i]['CounterRecID'];
				echo $errCode.$errMsg."<br>";
				fnLogMessageToDb($errCode.$errMsg); }
			//The highest counter for a particular stack must match the stackHeight of the corresponding stack record
			if ($i > 0 and $countersRows[$i-1]['CounterHeight'] != $stackRows[$countersRows[$i-1]['StackNum']]['StackHeight']) {
				$errCode = "DCH-C";
				$errMsg = "counter height doesn't match stack for counter " . $countersRows[$i-1]['CounterRecID'];
				echo $errCode.$errMsg."<br>";
				fnLogMessageToDb($errCode.$errMsg); }
			//reset lastStackNum
			$lastStackNum = $countersRows[$i]['StackNum'];
		}
		else {
			if ($countersRows[$i]['CounterHeight'] != $countersRows[$i-1]['CounterHeight'] + 1) {
				$errCode = "DCH-B";
				$errMsg = "counter height not incrementing by 1 for counter " . $countersRows[$i]['CounterRecID'];
				echo $errCode.$errMsg."<br>";
				fnLogMessageToDb($errCode.$errMsg);
			}
		}
		//The Player ref for each counter must match one of the player refs in the Game record 
		if ($countersRows[$i]['PlayerRef'] != $gameRow['P1RecID'] and $countersRows[$i]['PlayerRef'] != $gameRow['P2RecID']) {
			$errCode = "DCH-F";
			$errMsg = "invalid player ref on counter " . $countersRows[$i]['CounterRecID'];
			echo $errCode.$errMsg."<br>";
			fnLogMessageToDb($errCode.$errMsg); }
	}
	//The highest counter for a particular stack must match the stackHeight of the corresponding stack record
	$i = count($countersRows);
	if ($i > 0 and $countersRows[$i-1]['CounterHeight'] != $stackRows[$countersRows[$i-1]['StackNum']]['StackHeight']) {
		$errCode = "DCH-D";
		$errMsg = "counter height doesn't match stack for counter " . $countersRows[$i-1]['CounterRecID'];
		echo $errCode.$errMsg."<br>";
		fnLogMessageToDb($errCode.$errMsg); }
	if (strlen($errCode)==0) {
		fnLogMessageToDb("No errors data errors found");
		echo "No errors data errors found";
	}
}
//End fnDataChecks
function fnErrorHandler($errorLevel,$errorMessage,$errorFile="",$errorLine="",$errorContext="") {
 	//fnLogMessageToDb("start fnErrorHandler");
	global $callString, $logLevel;
	$errString = date("Y-m-d H:i:s")." error level=>".$errorLevel."< error message=>".$errorMessage;
	if (strlen($errorFile) != 0) {
		$errString = $errString."< error file=>".$errorFile."< ";}
	if (strlen($errorLine) != 0) {
		$errString = $errString."< error line=".$errorLine."< "; }
	$errString = $errString."\n";
	if ($_SERVER['SERVER_NAME'] == 'localhost') {
		if (substr($callString,0,4)=="cron") {
			file_put_contents("C:/xampp/htdocs/stacks2/Logs2/comment_log.txt",$errString,FILE_APPEND); }
		else {
			file_put_contents("C:/xampp/htdocs/stacks2/Logs2/comment_log.txt",$errString,FILE_APPEND); }
	}
	else if ( substr($_SERVER['DOCUMENT_ROOT'],-6) == 'stacks') {
			file_put_contents($_SERVER["DOCUMENT_ROOT"]."/Logs/comment_log.txt",$errString,FILE_APPEND); }
		else {
			file_put_contents($_SERVER["DOCUMENT_ROOT"]."/stacks/Logs/comment_log.txt",$errString,FILE_APPEND); }
	
}
//End fnErrorHandler
function fnCommentLog($comment) {
	$commentString = date("Y-m-d H:i:s")." comment=>".$comment."< \n";
	
	if ($_SERVER['SERVER_NAME'] == 'localhost') {
		file_put_contents("C:/xampp/htdocs/stacks2/Logs2/comment_log.txt",$commentString,FILE_APPEND); }
	else if (substr($_SERVER['DOCUMENT_ROOT'],-6) == 'stacks') {
		file_put_contents($_SERVER["DOCUMENT_ROOT"]."/Logs/comment_log.txt",$commentString,FILE_APPEND); }
	else {
			 file_put_contents($_SERVER["DOCUMENT_ROOT"]."/stacks/Logs/comment_log.txt",$commentString,FILE_APPEND); 
		}
}
//End fnCommentLog

?>