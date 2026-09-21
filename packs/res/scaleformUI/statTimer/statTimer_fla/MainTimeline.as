package statTimer_fla
{
   import adobe.utils.*;
   import flash.accessibility.*;
   import flash.desktop.*;
   import flash.display.*;
   import flash.errors.*;
   import flash.events.*;
   import flash.external.*;
   import flash.filters.*;
   import flash.geom.*;
   import flash.globalization.*;
   import flash.media.*;
   import flash.net.*;
   import flash.net.drm.*;
   import flash.printing.*;
   import flash.profiler.*;
   import flash.sampler.*;
   import flash.sensors.*;
   import flash.system.*;
   import flash.text.*;
   import flash.text.engine.*;
   import flash.text.ime.*;
   import flash.ui.*;
   import flash.utils.*;
   import flash.xml.*;
   
   public dynamic class MainTimeline extends MovieClip
   {
      public var back:MovieClip;
      
      public var upStat:dm_up_tile;
      
      public function MainTimeline()
      {
         super();
      }
      
      public function setScore(red:*, blue:*) : *
      {
         this.upStat.setScore(red,blue);
      }
      
      public function updateTimer(timeLostSec:Number) : *
      {
         var _loc2_:* = new Date();
         var _loc3_:* = timeLostSec + _loc2_.getTime() / 1000;
         this.upStat.setTimeEnd(_loc3_);
      }
      
      public function setTeam(teamID:Number) : *
      {
         if(teamID == 1)
         {
            this.upStat.bg.team.gotoAndStop("team_red");
            return undefined;
         }
         if(teamID == 2)
         {
            this.upStat.bg.team.gotoAndStop("team_blue");
            return undefined;
         }
         this.upStat.bg.team.gotoAndStop("team_none");
      }
      
      public function updatePosition() : *
      {
         this.back.y = (Object(root).height - stage.stageHeight) / 2;
         this.upStat.y = (Object(root).height - stage.stageHeight) / 2;
      }
      
      public function onResizeScreen(event:Event) : *
      {
         this.updatePosition();
         setTimeout(this.updatePosition,10);
      }
      
      public function onPress(movieEvent:MouseEvent) : *
      {
         ExternalInterface.call("changeVisibleScore");
      }
      
      public function onRollOver(movieEvent:MouseEvent) : *
      {
         this.upStat.bg.gotoAndStop(2);
      }
      
      public function onRollOut(movieEvent:MouseEvent) : *
      {
         this.upStat.bg.gotoAndStop(1);
      }
   }
}

