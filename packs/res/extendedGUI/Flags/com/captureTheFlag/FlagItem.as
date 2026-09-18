package com.captureTheFlag
{
   import flash.display.MovieClip;
   import flash.text.TextField;
   
   public class FlagItem extends MovieClip
   {
      public var ClanName:TextField;
      
      public var Indicator:FlagIndicator;
      
      protected var _flagName:String;
      
      protected var _value:Number;
      
      protected var _myFlag:Boolean;
      
      public function FlagItem()
      {
         super();
      }
      
      public function set flagName(value:String) : *
      {
         if(value == this._flagName)
         {
            return;
         }
         this._flagName = value;
         this.Indicator.flagName = value;
         this.draw();
      }
      
      public function get flagName() : String
      {
         return this._flagName;
      }
      
      public function set value(val:Number) : *
      {
         if(val == this._value)
         {
            return;
         }
         this._value = val;
         this.Indicator.value = val;
         this.draw();
      }
      
      public function get value() : Number
      {
         return this._value;
      }
      
      public function set myFlag(value:Boolean) : *
      {
         this._myFlag = value;
         this.ClanName.text = value.toString();
         trace(value);
         this.draw();
      }
      
      public function get myFlag() : Boolean
      {
         return this._myFlag;
      }
      
      protected function draw() : *
      {
         this.ClanName.width = this.ClanName.textWidth + 6;
         this.ClanName.x = -this.ClanName.width - 5;
         this.Indicator.invalidate();
      }
   }
}

