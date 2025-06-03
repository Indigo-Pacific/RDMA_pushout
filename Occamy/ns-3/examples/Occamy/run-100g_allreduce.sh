echo "hello, this is an experienment in small spine topo"

cd ../../
SERVERS=16
LEAVES=8
SPINES=8
LINKS=1
SERVER_LEAF_CAP=100
LEAF_SPINE_CAP=100
LATENCY=10
BUFFER_PER_PORT_PER_GBPS=5.12


nPrior=2
requestFlowRate=200.0

bufferSize=$(python3 -c "print(int($BUFFER_PER_PORT_PER_GBPS*1024*8*$SERVER_LEAF_CAP))")
N_CORES=60
N=0
method_array=("DT" "ABM" "PUSHOUT" "pBuffer")
tcpProtocol_array=("DCTCP")

webLoad_array=(0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8)
requestSizeRate_array=(0.3)
for webLoad in ${webLoad_array[*]};do
    for requestSizeRate in ${requestSizeRate_array[*]};do        
        for tcpProtocol in ${tcpProtocol_array[*]};do
            for method in ${method_array[*]};do
                alpha=1.0
                if [ "$method" == "pBuffer" ]
                then
                    alpha=8.0
                fi
                if [ "$method" == "ABM" ]
                then
                    alpha=2.0
                fi
                while [[ $(ps aux|grep "occamy_100g_allreduce-optimized"|wc -l) -gt $N_CORES ]];do
                    sleep 10;
                    echo "waiting for cores, $N running..."
                done
                N=$(( $N+1 ))
                echo "./ns3 run \"examples/Occamy/occamy_100g_allreduce.cc --method=${method} --alpha=${alpha} --tcpProtocol=${tcpProtocol} --webLoad=${webLoad} --requestSizeRate=${requestSizeRate} --requestFlowRate=${requestFlowRate} --bufferSize=${bufferSize} --nPrior=${nPrior}\""
                ./ns3 run "examples/Occamy/occamy_100g_allreduce.cc --method=${method} --alpha=${alpha} --tcpProtocol=${tcpProtocol} --webLoad=${webLoad} --requestSizeRate=${requestSizeRate} --requestFlowRate=${requestFlowRate} --bufferSize=${bufferSize} --nPrior=${nPrior}" > /dev/null &
                sleep 2
                echo $N
            done
        done
    done
done

while [[ $(ps aux|grep "occamy_100g_allreduce-optimized"|wc -l) -gt 1 ]];do
	echo "Waiting for simulations to finish..."
	sleep 5
done


echo "##################################"
echo "#      FINISHED EXPERIMENTS      #"
echo "##################################"

