package com.captureTheFlag
{
   import flash.display.MovieClip;
   
   public class FlagList extends MovieClip
   {
      protected const ind_distns:* = 13;
      
      protected var _flags:Object = new Object();
      
      protected var _sortedArrayFlagNames:Array = new Array();
      
      public function FlagList()
      {
         super();
      }
      
      public function addFlag(flag:CircleFlagIndicator) : *
      {
         this._flags[flag.flagName] = flag;
      }
      
      public function buyedFlag(name:String, status:Boolean) : *
      {
         if(this._flags[name] != null)
         {
            if(status)
            {
               this._flags[name].value = 100;
               this._flags[name].myFlag = true;
               this._flags[name].onStatePlay();
            }
            else
            {
               this._flags[name].value = 0;
               this._flags[name].myFlag = true;
               this._flags[name].addBuy();
            }
         }
      }
      
      public function ByedFlags(Obj:Object) : *
      {
         var i:* = undefined;
         this._flags = new Object();
         this._sortedArrayFlagNames = new Array();
         for(i in Obj)
         {
            if(i != "flag_cost")
            {
               this._flags[i] = new CircleFlagIndicator();
               this._flags[i].flagName = i;
               if(Obj[i].active)
               {
                  this._flags[i].value = 100;
                  this._flags[i].myFlag = true;
                  this._flags[i].addShow();
               }
               else
               {
                  this._flags[i].value = 0;
                  this._flags[i].myFlag = true;
                  this._flags[i].addBuy();
                  this._flags[i].addShow();
               }
               this._sortedArrayFlagNames.push(i);
            }
         }
         this._sortedArrayFlagNames.sort();
         this.drawFlags();
      }
      
      public function flagsData(Obj:Object) : *
      {
         var i:* = undefined;
         this._flags = new Object();
         this._sortedArrayFlagNames = new Array();
         for(i in Obj)
         {
            this._flags[i] = new CircleFlagIndicator();
            this._flags[i].flagName = i;
            this._flags[i].value = Obj[i].height;
            this._flags[i].myFlag = Obj[i].is_my_flag;
            this._sortedArrayFlagNames.push(i);
         }
         this._sortedArrayFlagNames.sort();
         this.drawFlags();
      }
      
      public function setFlagValue(name:String, value:Number) : *
      {
         if(this._flags[name] != null)
         {
            this._flags[name].value = value;
         }
         else
         {
            this.addFlagByName(name,value);
         }
      }
      
      public function setFlagOwner(name:String, value:Boolean) : *
      {
         if(this._flags[name] != null)
         {
            this._flags[name].myFlag = value;
         }
         else
         {
            this.addFlagByName(name,0,value);
         }
      }
      
      protected function addFlagByName(name:String, value:Number = -1, owner:Boolean = false) : *
      {
         this._flags[name] = new CircleFlagIndicator();
         this._flags[name].flagName = name;
         this._flags[name].value = value;
         this._flags[name].myFlag = owner;
         this._sortedArrayFlagNames.push(name);
         this._sortedArrayFlagNames.sort();
         this.drawFlags();
      }
      
      protected function drawFlags() : *
      {
         var i:* = undefined;
         var tmp:String = null;
         for(var j:* = numChildren - 1; j >= 0; j--)
         {
            removeChildAt(i);
         }
         var Y:Number = this.ind_distns;
         var X:Number = 0;
         var k:int = 0;
         for(i in this._sortedArrayFlagNames)
         {
            tmp = this._sortedArrayFlagNames[i].toString();
            this._flags[tmp].y = Y;
            this._flags[tmp].x = X;
            X += 64;
            this.addChild(this._flags[tmp]);
            k++;
            if(k == 3)
            {
               Y += 50 + this.ind_distns;
               X = 0;
               k = 0;
            }
         }
      }
   }
}

