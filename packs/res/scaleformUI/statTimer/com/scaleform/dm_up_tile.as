package com.scaleform
{
   import flash.display.MovieClip;
   import flash.text.TextField;
   import flash.utils.clearInterval;
   import flash.utils.setInterval;
   
   public class dm_up_tile extends MovieClip
   {
      public var bg:MovieClip;
      
      public var tTimer:TextField;
      
      public var scoreRed:TextField;
      
      public var scoreBlue:TextField;
      
      public var endTimeGame:Number;
      
      private var interval:Number;
      
      public function dm_up_tile()
      {
         super();
         this.onLoad();
      }
      
      public function onLoad() : void
      {
         this.tTimer.text = "00:00";
         this.interval = setInterval(this.intCall,500);
      }
      
      private function intCall() : void
      {
         this.timerUpdate0();
         clearInterval(this.interval);
         this.interval = setInterval(this.intCall,500);
      }
      
      public function setScore(red:String, blue:String) : void
      {
         this.scoreRed.text = red;
         this.scoreBlue.text = blue;
      }
      
      public function setTimeEnd(endTimeGame:Number) : void
      {
         this.endTimeGame = endTimeGame;
      }
      
      public function printData() : void
      {
      }
      
      public function timerUpdate0() : void
      {
         var sec:* = undefined;
         var minut:* = undefined;
         var hours:* = undefined;
         var t:String = null;
         var my_date:Date = new Date();
         var timeLost:Number = this.endTimeGame - my_date.getTime() / 1000;
         if(timeLost < 1 || !timeLost)
         {
            this.tTimer.text = "00:00";
         }
         else
         {
            sec = Math.floor(timeLost);
            minut = Math.floor(sec / 60);
            hours = Math.floor(minut / 60);
            sec = String(sec % 60);
            if(sec.length < 2)
            {
               sec = "0" + sec;
            }
            minut = String(minut % 60);
            if(minut.length < 2)
            {
               minut = "0" + minut;
            }
            hours = String(hours % 24);
            if(hours.length < 2)
            {
               hours = "0" + hours;
            }
            t = ":";
            this.tTimer.text = minut + t + sec;
         }
      }
   }
}

