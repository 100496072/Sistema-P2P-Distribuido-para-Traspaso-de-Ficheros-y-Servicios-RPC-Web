program PRINTRPC {
   version PRINTRPCSERVER{
    int print_operation(string username<256>, string operation<256>, string date<256>, string hour<256>) = 1; 
   } = 1;
} = 100496633;