class com.scaleform.DM_title extends MovieClip
{
   var interval;
   var endTimeGame;
   var tMeScore;
   var tCountPlayers;
   var tLimitCountPlayers;
   var tTimer;
   function DM_title()
   {
      super();
   }
   function onLoad()
   {
      this.interval = setInterval(mx.utils.Delegate.create(this,this.intCall),500);
   }
   function intCall()
   {
      this.timerUpdate0();
      clearInterval(this.interval);
      this.interval = setInterval(mx.utils.Delegate.create(this,this.intCall),500);
   }
   function setDataX(mapInfo)
   {
      trace("setDataX " + mapInfo);
      this.endTimeGame = mapInfo.endTimeGame;
      this.tMeScore.text = mapInfo.MeScore;
      this.tCountPlayers.text = mapInfo.CountPlayers;
      this.tLimitCountPlayers.text = "/" + mapInfo.LimitCountPlayers;
   }
   function printData()
   {
      trace("foter printData this.endTimeGame:" + this.endTimeGame);
   }
   function timerUpdate0()
   {
      var _loc7_ = new Date();
      var _loc5_ = this.endTimeGame - _loc7_.getTime() / 1000;
      if(_loc5_ < 1 or !_loc5_)
      {
         this.tTimer.text = "00:00";
      }
      else
      {
         var _loc2_ = Math.floor(_loc5_);
         var _loc3_ = Math.floor(_loc2_ / 60);
         var _loc4_ = Math.floor(_loc3_ / 60);
         _loc2_ = String(_loc2_ % 60);
         if(_loc2_.length < 2)
         {
            _loc2_ = "0" + _loc2_;
         }
         _loc3_ = String(_loc3_ % 60);
         if(_loc3_.length < 2)
         {
            _loc3_ = "0" + _loc3_;
         }
         _loc4_ = String(_loc4_ % 24);
         if(_loc4_.length < 2)
         {
            _loc4_ = "0" + _loc4_;
         }
         var _loc6_ = ":";
         this.tTimer.text = _loc4_ + _loc6_ + _loc3_ + _loc6_ + _loc2_;
      }
   }
}
