package com.captureTheFlag
{
   import com.GameCommunication;
   import com.greensock.easing.*;
   import flash.display.MovieClip;
   import flash.display.Sprite;
   import flash.events.MouseEvent;
   import flash.events.TimerEvent;
   import flash.external.ExternalInterface;
   import flash.text.TextField;
   import flash.text.TextFieldAutoSize;
   import flash.utils.Timer;
   import flash.utils.setTimeout;
   import scaleform.clik.controls.Button;
   
   public class CaptureTheFlag extends Sprite
   {
      protected var _baseName:String = "";
      
      protected var _owner:String = "";
      
      protected var _status:int;
      
      protected var _timer:Timer;
      
      protected var _time_stop:Number = 0;
      
      protected var _time_duration:Number = 0;
      
      protected var _days:int = 0;
      
      protected var _hours:int = 0;
      
      protected var _mins:int = 0;
      
      protected var _secs:int = 0;
      
      protected const STATUS_FREE:* = 0;
      
      protected const STATUS_PACIFIC:* = 1;
      
      protected const STATUS_CAPTURE:* = 2;
      
      protected const ICO_SIZE:* = 20;
      
      protected const ANIM_DURATION:* = 0.2;
      
      public var Loc:Object = {
         "FREE":"База свободна",
         "START_ATTACK":"до начала захвата",
         "END_ATTACK":"до окончания захвата",
         "STOP_BUY":"Завершить покупку",
         "DAY0":"день",
         "DAY1":"дня",
         "DAY2":"дней"
      };
      
      protected const ONE_SECOND:* = 1000;
      
      public var BaseName:TextField;
      
      public var Clock:TextField;
      
      public var BaseOwner:TextField;
      
      public var SubClockField:TextField;
      
      public var StopBuyBtn:Button;
      
      public var frameR:MovieClip;
      
      public var frameL:MovieClip;
      
      public var frameOwnR:MovieClip;
      
      public var frameOwnL:MovieClip;
      
      public var doubleMoneySymb:MovieClip;
      
      public var moneyText:TextField;
      
      public var ListFlags:FlagList = new FlagList();
      
      protected var API:GameCommunication = new GameCommunication();
      
      public function CaptureTheFlag()
      {
         this._status = this.STATUS_FREE;
         this._timer = new Timer(this.ONE_SECOND);
         super();
         this._timer.addEventListener(TimerEvent.TIMER,this.timerTick);
         this.StopBuyBtn.visible = false;
         this.moneyText.visible = false;
         this.doubleMoneySymb.visible = false;
         this.StopBuyBtn.focused = 0;
         this.frameR.visible = false;
         this.frameL.visible = false;
         this.BaseName.autoSize = TextFieldAutoSize.CENTER;
         this.BaseName.wordWrap = true;
         this.BaseOwner.autoSize = TextFieldAutoSize.CENTER;
         this.BaseOwner.wordWrap = true;
         this.StopBuyBtn.addEventListener(MouseEvent.CLICK,this.stop_buy);
         ExternalInterface.addCallback("fully_update_component",this.setBase);
         ExternalInterface.addCallback("flag_height",this.setFlagValue);
         ExternalInterface.addCallback("flag_clan_owner",this.setFlagOwner);
         ExternalInterface.addCallback("change_status",this.setStatus);
         ExternalInterface.addCallback("leave_base",this.leaveBase);
         ExternalInterface.addCallback("base_owner",this.setOwner);
         ExternalInterface.addCallback("start_flag_buying",this.startByingFlags);
         ExternalInterface.addCallback("stop_flag_buying",this.stopByingFlags);
         ExternalInterface.addCallback("localized_resource",this.setLocalizedResourse);
         ExternalInterface.addCallback("buy_flag",this.sayBuyingFlag);
         addChild(this.ListFlags);
         setTimeout(this.getLocalizedResourse,50);
      }
      
      public function setMoneyIcon(cost:String, base_money:String, is_gold:Boolean) : *
      {
         this.moneyText.text = cost + "\n" + base_money;
         if(this.StopBuyBtn.visible)
         {
            this.moneyText.visible = true;
            this.doubleMoneySymb.visible = true;
         }
         this.doubleMoneySymb.setGold(is_gold);
      }
      
      protected function setLocalizedResourse(Obj:*) : *
      {
         this.Loc = Obj["CaptureTheFlag"];
         this.API.ready();
         this.draw();
      }
      
      protected function sayBuyingFlag(Obj:*) : *
      {
         this.ListFlags.buyedFlag(Obj.name,Obj.success);
      }
      
      protected function getLocalizedResourse() : *
      {
         this.API.getLocal(["CaptureTheFlag"]);
      }
      
      protected function stop_buy(e:MouseEvent) : *
      {
         this.StopBuyBtn.selected = false;
         this.StopBuyBtn.focused = 0;
         this.stopByingFlags();
         GameCommunication.come_back_to_earth();
      }
      
      protected function ClickBaseName(e:MouseEvent) : *
      {
         GameCommunication.go_to_main_plan();
      }
      
      protected function startByingFlags(Obj:*) : *
      {
         Object(root).stopResize();
         this.ListFlags.visible = true;
         this.ListFlags.ByedFlags(Obj);
         this.StopBuyBtn.visible = true;
         this.moneyText.visible = true;
         this.doubleMoneySymb.visible = true;
         this.moneyText.y = this.ListFlags.y + this.ListFlags.height + 12;
         this.doubleMoneySymb.y = this.moneyText.y + 3;
         this.StopBuyBtn.y = this.moneyText.y + this.moneyText.height + 3;
         this.BaseName.addEventListener(MouseEvent.CLICK,this.ClickBaseName);
         this.BaseName.addEventListener(MouseEvent.MOUSE_OVER,this.overBaseName);
         this.BaseName.addEventListener(MouseEvent.MOUSE_OUT,this.outBaseName);
         this.frameR.visible = true;
         this.frameR.y = this.BaseName.y;
         this.frameR.x = this.BaseName.x + this.BaseName.width / 2 + this.BaseName.textWidth / 2 + 4;
         this.frameL.visible = true;
         this.frameL.y = this.BaseName.y;
         this.frameL.x = this.BaseName.x + this.BaseName.width / 2 - this.BaseName.textWidth / 2 - 4;
      }
      
      protected function stopByingFlags() : *
      {
         Object(root).startResize();
         this.BaseName.removeEventListener(MouseEvent.CLICK,this.ClickBaseName);
         this.BaseName.removeEventListener(MouseEvent.MOUSE_OVER,this.overBaseName);
         this.BaseName.removeEventListener(MouseEvent.MOUSE_OUT,this.outBaseName);
         this.ListFlags.visible = false;
         this.StopBuyBtn.visible = false;
         this.moneyText.visible = false;
         this.doubleMoneySymb.visible = false;
         this.frameR.visible = false;
         this.frameL.visible = false;
      }
      
      protected function outBaseName(e:MouseEvent) : *
      {
         this.frameR.gotoAndStop("up");
         this.frameL.gotoAndStop("up");
      }
      
      protected function overBaseName(e:MouseEvent) : *
      {
         this.frameR.gotoAndStop("over");
         this.frameL.gotoAndStop("over");
      }
      
      protected function setOwner(owner:String) : *
      {
         this._owner = owner;
         this.BaseOwner.text = owner;
         this.draw();
      }
      
      protected function leaveBase() : *
      {
         this.visible = false;
         this._timer.stop();
      }
      
      protected function set status(value:int) : *
      {
         this._status = value;
         Object(root).validNotification();
         this.draw();
      }
      
      public function setFlagValue(Obj:*) : *
      {
         this.ListFlags.setFlagValue(Obj.name,Obj.height);
      }
      
      public function setFlagOwner(Obj:*) : *
      {
         this.ListFlags.setFlagOwner(Obj.name,Obj.is_my_flag);
      }
      
      protected function setTime(Obj:Number) : *
      {
         this.start(Obj);
      }
      
      protected function setStatus(Obj:*) : *
      {
         this.status = Obj.state_code;
         this.setTime(Obj.next_time);
      }
      
      public function setBase(Obj:*) : *
      {
         this.visible = true;
         this.BaseOwner.text = Obj.owner;
         this.BaseName.text = Obj.name;
         var date:Date = new Date();
         this._time_duration = Obj.next_time;
         this.ListFlags.flagsData(Obj.flags);
         this.status = Obj.state;
      }
      
      public function start(time_stop:Number) : *
      {
         var date:Date = new Date();
         this._time_stop = date.time / 1000 + time_stop;
         this._time_duration = time_stop;
         if(this._time_stop >= 0)
         {
            this.updateTimerText();
         }
         this._timer.start();
      }
      
      private function _getRussianDay(num:String) : String
      {
         var res:String = " ";
         switch(num)
         {
            case "0":
               res = this.Loc.DAY2;
               break;
            case "1":
               res = this.Loc.DAY0;
               break;
            case "2":
               res = this.Loc.DAY1;
               break;
            case "3":
               res = this.Loc.DAY1;
               break;
            case "4":
               res = this.Loc.DAY1;
               break;
            case "5":
               res = this.Loc.DAY2;
               break;
            case "6":
               res = this.Loc.DAY2;
               break;
            case "7":
               res = this.Loc.DAY2;
               break;
            case "8":
               res = this.Loc.DAY2;
               break;
            case "9":
               res = this.Loc.DAY2;
         }
         return res;
      }
      
      private function updateTimerText() : *
      {
         var sMin:String = null;
         var sSec:String = null;
         var sDay:String = null;
         var current_time:Number = NaN;
         var date:Date = new Date();
         current_time = this._time_stop - date.time / 1000;
         if(current_time <= 0)
         {
            this._timer.stop();
            current_time = 0;
            stage.visible = false;
         }
         else
         {
            stage.visible = true;
         }
         this._days = current_time / 60 / 60 / 24;
         if(this._days >= 1)
         {
            sDay = this._days.toString();
            this.Clock.text = this._days.toString() + " " + this._getRussianDay(sDay.charAt(sDay.length - 1));
            return;
         }
         this._hours = current_time / 60 / 60 % 24;
         this._mins = current_time / 60 % 60;
         this._secs = current_time % 60;
         if(this._mins.toString().length == 1)
         {
            sMin = "0" + this._mins;
         }
         else
         {
            sMin = this._mins.toString();
         }
         if(this._secs.toString().length == 1)
         {
            sSec = "0" + this._secs;
         }
         else
         {
            sSec = this._secs.toString();
         }
         this.Clock.text = this._days + this._hours + ":" + sMin + ":" + sSec;
      }
      
      protected function timerTick(e:TimerEvent) : *
      {
         this.updateTimerText();
      }
      
      public function Show() : *
      {
      }
      
      protected function draw() : *
      {
         var Y:Number = 0;
         this.BaseName.y = Y;
         Y += this.BaseName.height;
         this.BaseOwner.y = Y;
         Y += this.BaseOwner.height;
         this.Clock.y = Y;
         Y += this.Clock.height;
         this.frameOwnR.y = this.BaseOwner.y;
         this.frameOwnR.x = this.BaseOwner.x + this.BaseOwner.width / 2 + this.BaseOwner.textWidth / 2 + 16;
         this.frameOwnL.visible = true;
         this.frameOwnL.y = this.BaseOwner.y;
         this.frameOwnL.x = this.BaseOwner.x + this.BaseOwner.width / 2 - this.BaseOwner.textWidth / 2 - 16;
         if(this.frameOwnL.x < this.BaseOwner.x)
         {
            this.frameOwnL.x = this.BaseOwner.x;
         }
         if(this.frameOwnR.x > this.BaseOwner.x + this.BaseOwner.width)
         {
            this.frameOwnR.x = this.BaseOwner.x + this.BaseOwner.width;
         }
         this.frameOwnR.height = this.BaseOwner.height;
         this.frameOwnL.height = this.BaseOwner.height;
         this.frameR.height = this.BaseName.height;
         this.frameL.height = this.BaseName.height;
         switch(this._status)
         {
            case this.STATUS_FREE:
               this._timer.stop();
               this._time_stop = 0;
               this.Clock.text = this.Loc.FREE;
               this.SubClockField.text = "";
               this.SubClockField.visible = false;
               this.ListFlags.visible = false;
               break;
            case this.STATUS_PACIFIC:
               this.SubClockField.text = this.Loc.START_ATTACK;
               this.SubClockField.visible = true;
               this.SubClockField.y = Y;
               Y += this.SubClockField.height;
               this.ListFlags.visible = false;
               this.start(this._time_duration);
               break;
            case this.STATUS_CAPTURE:
               this.SubClockField.text = this.Loc.END_ATTACK;
               this.SubClockField.visible = true;
               this.SubClockField.y = Y;
               Y += this.SubClockField.height;
               this.ListFlags.visible = true;
               this.start(this._time_duration);
         }
         this.ListFlags.y = Y - 2;
         this.ListFlags.x = 14;
         Y += this.ListFlags.height;
         this.StopBuyBtn.y = Y + 14;
         this.StopBuyBtn.x = this.width / 2 - this.StopBuyBtn.width / 2;
         this.StopBuyBtn.label = this.Loc.STOP_BUY;
      }
      
      public function Hide() : *
      {
      }
   }
}

