#!/bin/bash

if [ -x '/usr/local/bin/gsed' ] ; then
	## OSX
	# GSED='/usr/local/bin/sed'
	GSED='/usr/local/bin/gsed'
else
	## LINUX
	GSED='/bin/sed'
fi

# COLOR="never"
COLOR="always"

## OBS.: a linha 24 são correções necessárias pois alguns grupos não 
##       seguiram o padrão de texto de saída conforme exemplo do enunciado:
# XXX está na fila do caixa
# XXX está sendo atendido(a)
# XXX vai para casa
# Gerente detectou deadlock, liberando YYY para atendimento
# Gerente detectou inanição, aumentando prioridade de ZZZ

cat $@ | \
$GSED -e 's/entrou na fila/está na fila do caixa/; s/saiu do caixa e foi embora/vai para casa/; s/foi para casa/vai para casa/; s/Vovó Vanda/Vanda/; s/Vovô Valter/Valter/; s/(Idos.) //; s/(Deficiente) //; s/(Comum) //; s/(Grávida) //; s/(Criança no colo) //; s/Cliente //; /.imulação/d; /finalizou/d ; /encerrou o expediente/d; /completou todas as visitas/d; /está verificando a situação/d; s/(fila:.*//; s/ (.*//;' | \
$GSED -e 's/{fila:.*//'       	  | \
$GSED -e 's/ .*fila.*/ ESPERA/I;'       \
	-e 's/ .*atendido.*/ USANDO/I;'   \
	-e 's/ .*casa.*/ LIBERA/I;'	  | \
$GSED -e 's/Gerente.*inanição.*de/DETECTOU INANICAO/I;'   \
	  -e 's/Gerente.*deadlock.*/DEADLOCK RECOVERY/I;' | \
$GSED -e 's/Maria/A/I;  s/Marcos/a/I'              \
      -e 's/Vanda/B/I;  s/Valter/b/I'              \
      -e 's/Paula/C/I; 	s/Pedro/c/I'               \
      -e 's/Sueli/D/I;  s/Silas/d/I;'            | \
GREP_COLOR='01;30' grep -E --color=$COLOR "ESPERA|$"   | \
GREP_COLOR='01;33' grep -E --color=$COLOR "USANDO|$"   | \
GREP_COLOR='01;32' grep -E --color=$COLOR "LIBERA|$"   | \
GREP_COLOR='01;35' grep -E --color=$COLOR "(DETECTOU)|(INANICAO)|$" | \
GREP_COLOR='01;36' grep -E --color=$COLOR "(DEADLOCK)|(RECOVERY)|$" | \
tee saida-cleanup.txt | \
awk '
{ 

	if( $2 ~ /ESPERA/ ){
		quer[$1]++
		inanicao[$1]=0
		
		print $0

		## progresso garantido
		quer_sum=0
		for(t in quer){
			quer_sum += quer[t]
		}
		if( quer_sum == 1 ){
			quer_progresso=$1
		}
		else{
			quer_progresso="varias"
		}

		## wait-wait sem signal
		if(quer[$1]>1) {
			printf "==> ERRO de cond_wait:\t\t thread "$1" esperando "quer[$1]"x\n"
		}

	}

	## VERIFICA SE HÁ DEADLOCK (pode não ter sido identificado)
	if( !DEADLOCK && ( (quer["A"]||quer["a"]) && (quer["B"]||quer["b"]) && (quer["C"]||quer["c"]) ) ){
		DEADLOCK=1
		print "==> INFO há deadlock"
	}
	else{
		DEADLOCK=0
	}	

	if( $2 ~ /RECOVERY/ ){
		print $0
		#DEADLOCK=1
		RECOVERY=1			
	}	

	if( $2 ~ /INANICAO/ ){
		print $0
		INANICAO=1			
	}			

	if( $2 ~ /USANDO/ ){

		usando[$1]++
			if(inanicao[$1] > 0)
				print $0"\t[inanicao: +"inanicao[$1]"]"
			else
				print $0
		quer[$1]--
		

		if( quer_progresso ~ /[AaBbCc]]/ ){
			if( quer_progresso == $1 )
				printf "==> INFO progresso garantido para a thread "$1"\n"
			else
				printf "==> ERRO progresso garantido:\t "$1" era a única esperando mas foi impedida e outra acessou\n"	
		}

		usos[$1]++
		uso_total++

		if ( DEADLOCK && !RECOVERY ) {
			printf "==> ERRO com espera circular:\t havia DEADLOCK, mas "$1" acessou seção crítica sem RECOVERY\n"
		}

			 if( ((usando["C"] && inanicao["C"]<1) || (usando["c"] && inanicao["c"]<1)) && (quer["B"] || quer["b"]) && (!RECOVERY) )
				print "==> ERRO nas prioridades:\t Cc usando antes de Bb"

			 if( ((usando["B"] && inanicao["B"]<1) || (usando["b"] && inanicao["b"]<2)) && (quer["A"] || quer["a"]) && (!RECOVERY) )
				print "==> ERRO nas prioridades:\t Bb usando antes de Aa"

			 if( ((usando["A"] && inanicao["A"]<1) || (usando["a"] && inanicao["a"]<2)) && (quer["C"] || quer["c"]) && (!RECOVERY) )
				print "==> ERRO nas prioridades:\t Aa usando antes de Cc"
		
		inanicao[$1]=0

		if (DEADLOCK && RECOVERY) {
			RECOVERY=0
		}

		mutex++
		if(mutex >= 2){
			printf "==> ERRO na exclusão mútua:\t "mutex" threads usando o recurso simultaneamente (mutex "
			for(t in usando){
				if(usando[t]) printf " "t
			}
			print ")"
		}
	}

	if( $2 ~ /LIBERA/ ){

			## VERIFICA SE HAVERIA INANICAO
			if( usando["A"] || usando["a"] ) {
			 	if( quer["B"] ) inanicao["B"]++
			 	if( quer["b"] ) inanicao["b"]++
			}
			

			if( usando["B"] || usando["b"] ) {
				if( quer["C"] ) inanicao["C"]++
				if( quer["c"] ) inanicao["c"]++
			}
			

			if( usando["C"] || usando["c"]  ) {
				if( quer["A"] ) inanicao["A"]++
				if( quer["a"] ) inanicao["a"]++
			}

		usando[$1]--
		mutex--
		print $0

	}	

}
END{
	max_uso=0
	for(t in usos){
		if( usos[t] > max_uso )
			max_uso = usos[t];
	}
	for(t in usos){
		if( usos[t] < max_uso )
			printf "==> ERRO na espera limitada:\t thread "t" utilizou o recurso apenas "usos[t]" de "max_uso" vezes \n"	
	}
}' | \
GREP_COLOR='01;31' grep -E --color=$COLOR  "ERRO.*|$" | \
GREP_COLOR='01;34' grep -E --color=$COLOR  "INFO.*|$" | \
# sed 's/(0/( /g; s/0)/ )/g' | \
tee /tmp/pthreads.txt

grep -E "ERRO" /tmp/pthreads.txt | $GSED 's/:.*//' | sort | uniq -c
cat $@ | $GSED 's/{fila:.*//' | grep -E "(entra)|(usando)|(libera)" | sort | uniq -c | awk '{print "\t"$1,$2}' | uniq

COLOR="never"