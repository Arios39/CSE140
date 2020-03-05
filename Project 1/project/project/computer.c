#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "computer.h"
#undef mips			/* gcc already has a def for mips */

unsigned int endianSwap(unsigned int);

void PrintInfo (int changedReg, int changedMem);
unsigned int Fetch (int);
void Decode (unsigned int, DecodedInstr*, RegVals*);
int Execute (DecodedInstr*, RegVals*);
int Mem(DecodedInstr*, int, int *);
void RegWrite(DecodedInstr*, int, int *);
void UpdatePC(DecodedInstr*, int);
void PrintInstruction (DecodedInstr*);

/*Globally accessible Computer variable*/
Computer mips;
RegVals rVals;

 typedef struct{
//no such thing as map in c :(
   char *Inst;
 
 } Instname;
 Instname opname;
void namedatabase(DecodedInstr* d ,int data){
    char *r[43];
    char *i[8];
    char *j[2];
    i[0]="addiu";
    i[1]="andi";
    i[2]="ori";
    i[3]="lui";
    i[4]="beq";
    i[5]="bne";
    i[6]="lw";
    i[7]="sw";
    j[0]="j";
    j[1]="jal";
    r[33]="addu";
    r[35]="subbu";
    r[0]="sll";
    r[2]="srl";
    r[36]="and";
    r[37]="or";
    r[42]="slt";
    r[8]="jr";
    if (d->type==I){
        opname.Inst=i[data];
    }
    if (d->type==J){
          opname.Inst=j[data];
      }
    if (d->type==R){
          opname.Inst=r[data];
      }
}
/*
 *  Return an initialized computer with the stack pointer set to the
 *  address of the end of data memory, the remaining registers initialized
 *  to zero, and the instructions read from the given file.
 *  The other arguments govern how the program interacts with the user.
 */

void InitComputer (FILE* filein, int printingRegisters, int printingMemory,
  int debugging, int interactive) {
    int k;
    unsigned int instr;

    /* Initialize registers and memory */

    for (k=0; k<32; k++) {
        mips.registers[k] = 0;
    }
    
    /* stack pointer - Initialize to highest address of data segment */
    mips.registers[29] = 0x00400000 + (MAXNUMINSTRS+MAXNUMDATA)*4;

    for (k=0; k<MAXNUMINSTRS+MAXNUMDATA; k++) {
        mips.memory[k] = 0;
    }

    k = 0;
    while (fread(&instr, 4, 1, filein)) {
	/*swap to big endian, convert to host byte order. Ignore this.*/
        mips.memory[k] = ntohl(endianSwap(instr));
        k++;
        if (k>MAXNUMINSTRS) {
            fprintf (stderr, "Program too big.\n");
            exit (1);
        }
    }

    mips.printingRegisters = printingRegisters;
    mips.printingMemory = printingMemory;
    mips.interactive = interactive;
    mips.debugging = debugging;
}

unsigned int endianSwap(unsigned int i) {
    return (i>>24)|(i>>8&0x0000ff00)|(i<<8&0x00ff0000)|(i<<24);
}

/*
 *  Run the simulation.
 */
void Simulate () {
    char s[40];  /* used for handling interactive input */
    unsigned int instr;
    int changedReg=-1, changedMem=-1, val;
    DecodedInstr d;
    
    /* Initialize the PC to the start of the code section */
    mips.pc = 0x00400000;
    while (1) {
        if (mips.interactive) {
            printf ("> ");
            fgets (s,sizeof(s),stdin);
            if (s[0] == 'q') {
                return;
            }
        }
        /* Fetch instr at mips.pc, returning it in instr */
        instr = Fetch (mips.pc);
        if (instr==0){
            exit(0);
        }
        //stops program if the instruction is 0
        printf ("Executing instruction at %8.8x: %8.8x\n", mips.pc, instr);

        /* 
	 * Decode instr, putting decoded instr in d
	 * Note that we reuse the d struct for each instruction.
	 */
        Decode (instr, &d, &rVals);

        /*Print decoded instruction*/
        PrintInstruction(&d);

        /* 
	 * Perform computation needed to execute d, returning computed value 
	 * in val 
	 */
        val = Execute(&d, &rVals);

	UpdatePC(&d,val);

        /* 
	 * Perform memory load or store. Place the
	 * address of any updated memory in *changedMem, 
	 * otherwise put -1 in *changedMem. 
	 * Return any memory value that is read, otherwise return -1.
         */
        val = Mem(&d, val, &changedMem);

        /* 
	 * Write back to register. If the instruction modified a register--
	 * (including jal, which modifies $ra) --
         * put the index of the modified register in *changedReg,
         * otherwise put -1 in *changedReg.
         */
        RegWrite(&d, val, &changedReg);

        PrintInfo (changedReg, changedMem);
    }
}

/*
 *  Print relevant information about the state of the computer.
 *  changedReg is the index of the register changed by the instruction
 *  being simulated, otherwise -1.
 *  changedMem is the address of the memory location changed by the
 *  simulated instruction, otherwise -1.
 *  Previously initialized flags indicate whether to print all the
 *  registers or just the one that changed, and whether to print
 *  all the nonzero memory or just the memory location that changed.
 */
void PrintInfo ( int changedReg, int changedMem) {
    int k, addr;
    printf ("New pc = %8.8x\n", mips.pc);
    if (!mips.printingRegisters && changedReg == -1) {
        printf ("No register was updated.\n");
    } else if (!mips.printingRegisters) {
        printf ("Updated r%2.2d to %8.8x\n",
        changedReg, mips.registers[changedReg]);
    } else {
        for (k=0; k<32; k++) {
            printf ("r%2.2d: %8.8x  ", k, mips.registers[k]);
            if ((k+1)%4 == 0) {
                printf ("\n");
            }
        }
    }
    if (!mips.printingMemory && changedMem == -1) {
        printf ("No memory location was updated.\n");
    } else if (!mips.printingMemory) {
        printf ("Updated memory at address %8.8x to %8.8x\n",
        changedMem, Fetch (changedMem));
    } else {
        printf ("Nonzero memory\n");
        printf ("ADDR	  CONTENTS\n");
        for (addr = 0x00400000+4*MAXNUMINSTRS;
             addr < 0x00400000+4*(MAXNUMINSTRS+MAXNUMDATA);
             addr = addr+4) {
            if (Fetch (addr) != 0) {
                printf ("%8.8x  %8.8x\n", addr, Fetch (addr));
            }
        }
    }
}

/*
 *  Return the contents of memory at the given address. Simulates
 *  instruction fetch. 
 */
unsigned int Fetch ( int addr) {
    return mips.memory[(addr-0x00400000)/4];
}


char Opcodedetector(DecodedInstr* d){
    int value = d->op;
    int i_values[9]={9,12,13,15,4,5,35,43};
    int j_values[2]={2,3};
    if (value ==0){
        d->type=R;
        return 'R';
    }
    for(int i =0; i<8;i++){
        if (value==i_values[i]){
             d->type=I;
            namedatabase(d, i);
            return 'I';
        }
    }
    for(int i =0; i<2;i++){
        if (value==j_values[i]){
           d->type=J;
            namedatabase(d, i);
            return 'J';
        }
    }
    return 'N';
}
void Jint(unsigned int instr, DecodedInstr* d, RegVals* rVals){
   instr=instr<<6;
    instr=instr>>6;
    d->regs.j.target = (instr<<2);
    // printf("This is the target Value %d \n", d->regs.j.target);

    
}
int signextimm(int value){
        value=value|4294901760;
        //we extend the bits to 0xffff0000 to get 2's complement of the signextened in the i istruction //https://stackoverflow.com/questions/2689028/need-fastest-way-to-convert-2s-complement-to-decimal-in-c?rq=1
    return value;
}
void Iint(unsigned int instr, DecodedInstr* d, RegVals* rVals){
    d->regs.i.addr_or_immed = instr&0xffff;
    //gets the last 16 bits of int
    if ( d->regs.i.addr_or_immed>32767){
        d->regs.i.addr_or_immed=signextimm(d->regs.i.addr_or_immed);
    }
    instr = instr>>16;
    d->regs.i.rt=instr & 31;
    //rVals->R_rt=d->regs.i.rt;
    instr = instr>>5;
       d->regs.i.rs=instr & 31;
   // rVals->R_rs=d->regs.i.rs;

}
void Rint(unsigned int instr, DecodedInstr* d, RegVals* rVals){
    d->regs.r.funct= instr<<25;
    d->regs.r.funct=d->regs.r.funct>>25;
    namedatabase(d, d->regs.r.funct);
    instr=instr>>6;
    //gets rid of the funct and will shorten our shifts
    d->regs.r.shamt=instr & 31;
    instr = instr >> 5;
    d->regs.r.rd=instr & 31;
   // rVals->R_rd=d->regs.r.rd;
    instr = instr >> 5;
    d->regs.r.rt=instr & 31;
      //  rVals->R_rt=d->regs.r.rt;
    instr = instr >> 5;
    d->regs.r.rs=instr & 31;
      //  rVals->R_rs=d->regs.r.rs;

 //  printf("This is the func Value %d \n", d->regs.r.funct);
//printf("This is the shamt Value %d \n", d->regs.r.shamt);
  //  printf("This is the rd Value %d \n", d->regs.r.rd);
    //printf("This is the rt Value %d \n", d->regs.r.rt);
   // printf("This is the rs Value %d \n", d->regs.r.rs);
}
/* Decode instr, returning decoded instruction. */
void Decode ( unsigned int instr, DecodedInstr* d, RegVals* rVals) {
   char inst;
    d->op=instr>>26;
   inst = Opcodedetector(d);
    switch (inst){
        case 'R':
            Rint(instr,d,rVals);
            rVals->R_rs = mips.registers[d->regs.r.rs];
                rVals->R_rt = mips.registers[d->regs.r.rt];
                rVals->R_rd = mips.registers[d->regs.r.rd];
            break;
            case 'I':
     Iint(instr,d,rVals);
            rVals->R_rs = mips.registers[d->regs.i.rs];
            rVals->R_rt = mips.registers[d->regs.i.rt];
                    break;
            case 'J':
            Jint(instr,d,rVals);
                    break;
            
         default :
            exit(0);
    
    }

}

/*
 *  Print the disassembled version of the given instruction
 *  followed by a newline.
 */
void PrintInstruction ( DecodedInstr* d) {
    switch (d->type) {
        case R:
            if(d->regs.r.funct==33||d->regs.r.funct==35||d->regs.r.funct==36||d->regs.r.funct==37||d->regs.r.funct==42){
printf("%s\t$%d, $%d, $%d\n", opname.Inst, d->regs.r.rd, d->regs.r.rs, d->regs.r.rt);
            }
            if(d->regs.r.funct==0||d->regs.r.funct==2){
            printf("%s\t$%d, $%d, %d\n", opname.Inst, d->regs.r.rd, d->regs.r.rs, d->regs.r.shamt);
                        }
            if(d->regs.r.funct==8){
                       printf("%s\t$%d\n", opname.Inst, d->regs.r.rs);
                                   }
            break;
                case I:
            if(d->op==4||d->op==5){
                                printf("%s\t$%d, $%d, 0x%08x\n", opname.Inst, d->regs.i.rs, d->regs.i.rt, (mips.pc + 4 + (d->regs.i.addr_or_immed << 2)));
                                }
            if(d->op==9){
            printf("%s\t$%d, $%d, %d\n", opname.Inst,d->regs.i.rt,d->regs.i.rs,d->regs.i.addr_or_immed);
            }
            if(d->op==12||d->op==13){
                  printf("%s\t$%d, $%d, 0x%x\n", opname.Inst, d->regs.i.rt, d->regs.i.rs, d->regs.i.addr_or_immed);
                   }
            if(d->op==15){
                                          printf("%s\t$%d, 0x%x\n", opname.Inst,d->regs.i.rt,d->regs.i.addr_or_immed);
                                          }
         
            if(d->op==35||d->op==43){
                       printf("%s\t$%d, %d($%d)\n", opname.Inst,d->regs.i.rt,d->regs.i.addr_or_immed,d->regs.i.rs);
                       }
        
            
                        break;
        case J:
            printf("%s\t0x%08x\n", opname.Inst,d->regs.j.target);
                        break;
            
        default:
            break;
    }

}
int Rexecute(DecodedInstr* d, RegVals* rVals){
     unsigned int val=0;
    //Sll operation
    if(d->regs.r.funct==0){
        val = rVals->R_rt << d->regs.r.shamt;
    }
    //SRL op
    if(d->regs.r.funct==2){
        val = rVals->R_rt >> d->regs.r.shamt;
    }
    //jr
    if(d->regs.r.funct==8){
           val = rVals->R_rs;
       }
    //Addu
    if(d->regs.r.funct==33){
            val = rVals->R_rs + rVals->R_rt;
        }
    //SUbu
      if(d->regs.r.funct==35){
              val = rVals->R_rs - rVals->R_rt;
          }
    //AND
        if(d->regs.r.funct==36){
                val = rVals->R_rs & rVals->R_rt;
            }
    //OR
           if(d->regs.r.funct==42){
                   val = (rVals->R_rs - rVals->R_rt) < 0;
               }
    //SLT
    if(d->regs.r.funct==36){
                val = rVals->R_rs | rVals->R_rt;
            }
    
    return val;
}

int Iexecute(DecodedInstr* d, RegVals* rVals){
  unsigned int val=0;
       // BEQ https://en.wikipedia.org/wiki/%3F:
    if(d->op==4){
        if(rVals->R_rs == rVals->R_rt){
            val = (d->regs.i.addr_or_immed << 2);
        }
        else{
            val=0;
        }
         }
       // BNE https://en.wikipedia.org/wiki/%3F:
    if( d->op==5){
        if(rVals->R_rs != rVals->R_rt){
            val = (d->regs.i.addr_or_immed << 2);
        }
        else{
            val=0;
        }
    }
       // ADDIU
    if (d->op==9){
        val = rVals->R_rs + d->regs.i.addr_or_immed;
    }
       // ANDI
    if (d->op==12){
         val = rVals->R_rs & d->regs.i.addr_or_immed;
     }
    //ORI
    if (d->op==13){
            val = rVals->R_rs | d->regs.i.addr_or_immed;
        }
    //LUI
    if (d->op==15){
            val = d->regs.i.addr_or_immed << 16;
    }
    //lw
    if (d->op==35){
               val = rVals->R_rs + d->regs.i.addr_or_immed;;
       }
    //sw
    if (d->op==43){
        val = rVals->R_rs + d->regs.i.addr_or_immed;
    }
        

    return val;
}


int Jexecute(DecodedInstr* d, RegVals* rVals){
   unsigned int val=0;
    if(d->op==3){
            val = mips.pc+4;
    }
       return val;
}



int Execute ( DecodedInstr* d, RegVals* rVals) {
    int value=0;
    switch(d->type){
        case R:
            value = Rexecute(d,rVals);
            break;
           case I:
            value = Iexecute(d,rVals);
            break;
   case J:
    value = Jexecute(d,rVals);
    break;
    
    default:
            break;
    }

    
  return value;
}

/* 
 * Update the program counter based on the current instruction. For
 * instructions other than branches and jumps, for example, the PC
 * increments by 4 (which we have provided).
 */
void UpdatePC ( DecodedInstr* d, int val) {
    mips.pc+=4;
    /* Your code goes here */
    ///jr
 if(d->op==0&&d->regs.r.funct == 8){
        mips.pc = val;
    }
    //beq
    if (d->op==4||d->op==5){
        //val was already updated with the execute inst
      mips.pc +=val;
     }
    //jump inst
 if (d->op==2||d->op==3){
mips.pc = d->regs.j.target;
    }

}

/*
 * Perform memory load or store. Place the address of any updated memory 
 * in *changedMem, otherwise put -1 in *changedMem. Return any memory value 
 * that is read, otherwise return -1. 
 *
 * Remember that we're mapping MIPS addresses to indices in the mips.memory 
 * array. mips.memory[0] corresponds with address 0x00400000, mips.memory[1] 
 * with address 0x00400004, and so forth.
 *
 */

int Mem( DecodedInstr* d, int val, int *changedMem) {
    /* Your code goes here */
    //if no change in mem return-1
   *changedMem = -1;
    if (d->op==35){
        if( 0x00401000>val|| val>0x00403fff ||val%4!=0){
            printf( "Memory Access Exception at [0x%x]: address [0x%08x]", val, mips.pc);
            exit(0);
        }
        else{
        return Fetch(val);
        }
    }
    if (d->op==43){

        if( 0x00401000>val|| val>0x00403fff ||val%4!=0){
                printf( "Memory Access Exception at [0x%x]: address [0x%08x]", val, mips.pc);
            exit(0);
        }
        else{
       mips.memory[Fetch(val)]=rVals.R_rt;
           *changedMem = val;
        }
    }
    return val;
}


/* 
 * Write back to register. If the instruction modified a register--
 * (including jal, which modifies $ra) --
 * put the index of the modified register in *changedReg,
 * otherwise put -1 in *changedReg.
 */
void RegWrite( DecodedInstr* d, int val, int *changedReg) {
    /* Your code goes here */
    *changedReg = -1;
//jr is the only r inst that doesnt take rd(write back)
    if (d->regs.r.funct==8&&d->type==R){
        *changedReg = -1;
    }
    if (d->regs.r.funct!=8&&d->type==R){
         mips.registers[d->regs.r.rd] = val;
         *changedReg = d->regs.r.rd;
    }
    //jal inst
    if(d->op==3){
        mips.registers[31] = val;
        //return address 31
        *changedReg = 31;
    }
    if((d->op==9||d->op==12||d->op==13||d->op==15||d->op==35)){
     
        mips.registers[d->regs.i.rt] = val;
        *changedReg = d->regs.i.rt;
        
    }
    
    
}
