function fnNegaMax($gameKey,$maxDepth,$currentDepth,$depthZeroMove="") {
	global $logLevel;

	fnLogMessageToDb("Start fnNegaMax. GK=" . $gameKey . " MD=" . $maxDepth . " CD=" . $currentDepth);
	
	//the response from this function will be returned in this array (unless an error is found)
	$returnArray="";
	$errCode="";
	
	//depthZeroMove is one of the original moves when currentDepth was zero. The best move is one of these moves.
	//To begin with depthZeroMove is empty but if currentDepth is >0 then depthZeroMove must not be blank
	if ($currentDepth>0 And is_string($depthZeroMove)) {
		$errCode = 'NMX-7';
		$errMsg = "currentDepth is $currentDepth and depthZeroMove is still blank"; 
		fnLogMessageToDb ($errCode.$errMsg); }
	
	if (strlen($errCode)==0){
		//find out if gameKey is GameRecId or Nmx hash key
		if (strlen($gameKey)<20) {
			$gameRecID=$gameKey; }
		else {
			$gameRecID = 0;
			$hashKey = $gameKey; }

		//get the relevant game, stacks and counters records
		if ($gameRecID>0) {
			//get game record using GameRecID
			if ($logLevel>1){fnLogMessageToDb("(fnNegaMax) get game record using GameRecID");}
			$gameRow = fnQrySelectGameByID($gameRecID);
			if (is_string($gameRow)) {
				$errCode = "NMX-1";
				$errMsg = $gameRow; 
				fnLogMessageToDb ($errCode.$errMsg); }
		}
		else {
			if ($logLevel>1){ fnLogMessageToDb("(fnNegaMax) get game record using hashKey");}
			$gameRow = fnQrySelectNmxGame($hashKey);
			if (is_string($gameRow)) {
				$errCode = "NMX-2";
				$errMsg = $gameRow;	
				fnLogMessageToDb ($errCode.$errMsg); }
		}
	}

	if (strlen($errCode)==0){
		//check if gone to required depth or game is over
		if ($gameRow['Winner']>0) {
			/*If the game has been won then return large score. The score is calulated from the next player's 
			perspective and the winner is always the last player so winning score is -ve */
			if ($logLevel>1){ fnLogMessageToDb("(fnNegaMax) winner so end of negamax. depth=" . $currentDepth . "/" . $maxDepth . " winner=" . $gameRow['Winner'] ); }
			$returnArray = array(-999,$depthZeroMove[1],$depthZeroMove[2],$depthZeroMove); 
		}
		elseif ($currentDepth==$maxDepth) {
			 if ($logLevel>1){ fnLogMessageToDb("(fnNegaMax) reached max depth so end of negamax. depth=" . $currentDepth . "/" . $maxDepth . " winner=" . $gameRow['Winner'] ); }
			//score game
			$fnResp=fnEvaluateGame($gameKey, $gameRow);
			if (is_string($fnResp)){
				$errCode="NMX-3";
				$errMsg=$fnResp;
				fnLogMessageToDb($errCode.$errMsg); }
			else {
				//return score and null values for moveNum and movePos
				$returnArray = array($fnResp,$depthZeroMove[1],$depthZeroMove[2],$depthZeroMove); }
		}
	}
	if (strlen($errCode)==0 And is_string($returnArray)){
		//no errors and returnArray has been set so set up for next step

		//initialise moveNum, movePos, best move and best score
		$movePos = -1; $moveNum = -1;
		$bestMove = array($moveNum, $movePos);
		$bestScore = -99999;

		//get all the possible moves for the game
		$fnResp = fnGetMoves($gameKey, $gameRow);
		if (is_string($fnResp)){
			$errCode="NMX-4";
			$errMsg=$fnResp;
			fnLogMessageToDb($errCode.$errMsg); }
		else {
			$movesList = $fnResp; 
			//process each of the moves in the list
			if ($logLevel>2){ fnLogMessageToDb('(fnNegaMax) process each of the moves in the list. count=' . count($movesList)); }
			foreach ($movesList as $aMove) {
				//$aMove[0] = $gameKey, [1]=moveNum, [2]=movePos
				$fnResp = fnProcessNmxMove ($aMove[0],$aMove[1],$aMove[2], $gameRow);
				if (substr($fnResp,0,3)=="PNM"){
					$errCode="NMX-5";
					$errMsg=$fnResp;
					fnLogMessageToDb($errCode.$errMsg); 
					break; }
				else {
					//fnProcessNmxMove has returned a new gameKey so use it to recursively call fnNegaMax
					$newGameKey = $fnResp;
					//if currentDepth is zero then save depthZeroMove
					if ($currentDepth==0){$depthZeroMove = $aMove; }
					fnLogMessageToDb('Recursive call to fnNegaMax. currentDepth+1=' . ($currentDepth+1));
					$fnResp = fnNegaMax($newGameKey,$maxDepth,$currentDepth+1,$depthZeroMove);
					if (is_string($fnResp)) {
						$errCode="NMX-6";
						$errMsg=$fnResp;
						fnLogMessageToDb($errCode.$errMsg); }
					else {
						$currentScore = 0 - $fnResp[0];
						//update best score and best move
						if ($logLevel > 1){fnLogMessageToDb("Checking score. bestScore= $bestScore curScore= $currentScore currMoveNum= $aMove[1] currMovePos= $aMove[2]"); }
						if ($currentScore > $bestScore) {
							$bestScore = $currentScore;
							$returnArray[0] = $bestScore;
							$returnArray[1] = $depthZeroMove[1]; //moveNum
							$returnArray[2] = $depthZeroMove[2]; //movePos
							if ($logLevel>2){ fnLogMessageToDb("New best score is {$returnArray[0]} new best movenum is {$returnArray[1]} and move pos is {$returnArray[2]}") ;}
						}
						if ($bestScore == 999) {
							//the last move is a winner for player 2 so stop searching
							break;
						}
					}
				}
			} //end foreach loop
		}
	}
	
	//finished - return error code or return array
	if (strlen($errCode) > 0) {
		fnLogMessageToDb("fnNegaMax finished. Return error code: " . $errCode . $errMsg);
		return $errCode.$errMsg; }
	else {
		fnLogMessageToDb("fnNegaMax finished. Return bestScore=" . $returnArray[0] . " and bestMove=" . $returnArray[1] . "/" . $returnArray[2]) ;
		return $returnArray; }

}
//End fnNegaMax