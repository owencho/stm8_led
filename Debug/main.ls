   1                     ; C Compiler for STM8 (COSMIC Software)
   2                     ; Parser V4.12.1 - 30 Jun 2020
   3                     ; Generator (Limited) V4.4.12 - 02 Jul 2020
  44                     ; 69 main() {
  46                     	switch	.text
  47  0000               _main:
  51                     ; 71 	GPIO_Init(GPIOD, GPIO_PIN_ALL ,GPIO_MODE_OUT_PP_HIGH_SLOW);
  53  0000 4bd0          	push	#208
  54  0002 4bff          	push	#255
  55  0004 ae500f        	ldw	x,#20495
  56  0007 cd0000        	call	_GPIO_Init
  58  000a 85            	popw	x
  59                     ; 72 	GPIO_WriteHigh(GPIOD,GPIO_PIN_4);
  61  000b 4b10          	push	#16
  62  000d ae500f        	ldw	x,#20495
  63  0010 cd0000        	call	_GPIO_WriteHigh
  65  0013 84            	pop	a
  66  0014               L12:
  67                     ; 73 	while (1);
  69  0014 20fe          	jra	L12
  82                     	xdef	_main
  83                     	xref	_GPIO_WriteHigh
  84                     	xref	_GPIO_Init
 103                     	end
