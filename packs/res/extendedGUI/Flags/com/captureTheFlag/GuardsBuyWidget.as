package com.captureTheFlag
{
   import com.GameCommunication;
   import flash.display.MovieClip;
   import flash.events.MouseEvent;
   import flash.external.ExternalInterface;
   import flash.text.TextField;
   import scaleform.clik.controls.Button;
   
   public class GuardsBuyWidget extends MovieClip
   {
      public var GuardLabel:TextField;
      
      public var GuardianText:TextField;
      
      public var BuyGuardian:Button;
      
      public var StopBuyGuard:Button;
      
      public var doubleMoneySymb:MovieClip;
      
      public var moneyText:TextField;
      
      protected var _currentCount:int = 0;
      
      protected var _maxCount:int = 10;
      
      protected var _stateBuying:Boolean = false;
      
      protected const ICO_SIZE:* = 20;
      
      public function GuardsBuyWidget()
      {
         super();
         ExternalInterface.addCallback("start_guardians_buying",this.startBuying);
         ExternalInterface.addCallback("deactivate_guardians_buying",this.deactivateBuying);
         ExternalInterface.addCallback("activate_guardians_buying",this.activateBuying);
         ExternalInterface.addCallback("on_buy_guard",this.onBuyedGuard);
         ExternalInterface.addCallback("stop_guardians_buying",this.stopBuying);
         ExternalInterface.addCallback("on_change_guard_count",this.updateGuardCount);
         this.BuyGuardian.addEventListener(MouseEvent.CLICK,this.onBuyGuard);
         this.StopBuyGuard.addEventListener(MouseEvent.CLICK,this.onStopBuy);
      }
      
      public function setMoneyIcon(cost:String, base_money:String, is_gold:Boolean) : *
      {
         this.moneyText.text = cost + "\n" + base_money;
         this.doubleMoneySymb.setGold(is_gold);
      }
      
      private function updateGuardCount(arg:Number) : *
      {
         this._currentCount = arg;
         this.updateText();
      }
      
      private function onBuyGuard(e:MouseEvent) : *
      {
         this.BuyGuardian.selected = false;
         this.BuyGuardian.focused = 0;
         this.BuyGuardian.enabled = false;
         GameCommunication.buy_guardian();
      }
      
      private function onStopBuy(e:MouseEvent) : *
      {
         this.stopBuying();
         GameCommunication.stop_guardians_buying();
      }
      
      protected function onBuyedGuard(result:*) : *
      {
         if(result)
         {
            ++this._currentCount;
            this.updateText();
         }
         if(this._stateBuying)
         {
            this.BuyGuardian.enabled = true;
         }
      }
      
      private function activateBuying() : *
      {
         this._stateBuying = true;
         this.BuyGuardian.enabled = true;
      }
      
      private function deactivateBuying() : *
      {
         this._stateBuying = false;
         this.BuyGuardian.enabled = false;
      }
      
      private function updateText() : *
      {
         this.GuardianText.text = this._currentCount + " / " + this._maxCount;
      }
      
      private function startBuying(Obj:*) : *
      {
         Object(root).stopResize();
         this.GuardLabel.text = Object(root).base.MainFlags.Loc.GUARDS;
         this.BuyGuardian.enabled = true;
         this.StopBuyGuard.label = Object(root).base.MainFlags.Loc.STOP_BUY;
         this.visible = true;
         this.y = Object(root).base.MainFlags.y + Object(root).base.MainFlags.SubClockField.y * Object(root).base.MainFlags.scaleY + Object(root).base.MainFlags.SubClockField.height * Object(root).base.MainFlags.scaleY + 10;
         this._currentCount = Obj.current_count;
         this._maxCount = Obj.max_count;
         this.updateText();
      }
      
      private function stopBuying() : *
      {
         Object(root).startResize();
         this.visible = false;
         this.StopBuyGuard.selected = false;
         this.BuyGuardian.enabled = false;
      }
   }
}

