package Flags_fla
{
   import flash.display.MovieClip;
   
   public dynamic class DoubleMoneySymb_5 extends MovieClip
   {
      public var money0:MovieClip;
      
      public var money1:MovieClip;
      
      public function DoubleMoneySymb_5()
      {
         super();
      }
      
      public function setGold(val:Boolean) : *
      {
         if(val)
         {
            this.money0.gotoAndStop("gold");
            this.money1.gotoAndStop("gold");
         }
         else
         {
            this.money0.gotoAndStop("money");
            this.money1.gotoAndStop("money");
         }
      }
   }
}

