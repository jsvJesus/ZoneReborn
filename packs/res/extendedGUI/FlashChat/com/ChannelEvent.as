package com
{
   import flash.events.Event;
   
   public class ChannelEvent extends Event
   {
      public static const CHANGE:String = "CHANGE";
      
      public var index:Number = -1;
      
      public var id:Number = -1;
      
      public var selected:Boolean = false;
      
      public function ChannelEvent(type:String, index:Number, selected:Boolean, id:Number)
      {
         super(type,false,false);
         this.index = index;
         this.selected = selected;
         this.id = id;
      }
      
      override public function toString() : String
      {
         return formatToString("ChannelEvent","type","index","selected","id");
      }
      
      override public function clone() : Event
      {
         return new ChannelEvent(type,this.index,this.selected,this.id);
      }
   }
}

