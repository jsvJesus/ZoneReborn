package com.dvalimona.utils
{
   public class NumberUtils
   {
      public static const DECIMAL:String = ".";
      
      public function NumberUtils()
      {
         super();
      }
      
      public static function RoundWithPrecision(arg1:Number, arg2:uint) : String
      {
         return arg1.toFixed(arg2);
      }
      
      public static function RoundWithPrecisionAndLeads(arg1:Number, arg2:uint, arg3:uint = 0, arg4:uint = 0) : String
      {
         return arg1.toFixed(3);
      }
   }
}

