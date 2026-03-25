#include "ft_malcolm.h"

int MAC_getter(char *to_convert, uint8_t *converted)
{
	char *tok[6];
	
	int i = 0;

	tok[i] = strtok(to_convert, ":");
	if (!tok[i])
		return 0;
	for (i = 1; i < 6; i++)
	{
		tok[i] = strtok(NULL, ":");
		if (!tok[i])
			return 0;
	}

	char *cursor;
	for (i = 0; i < 6; i++)
	{
		unsigned long val = strtoul(tok[i], &cursor, 16);
		if (*cursor != '\0')
			return 0;
		if (val > 0xFF)
    		return 0;
		converted[i] = val;
	}

	return 1;
}

int ip_getter(char *to_convert, uint8_t *converted)
{
	if (!inet_pton(AF_INET, to_convert, converted))
	{
		printf("invalide IP adress provided");
		return 0;
	}
	return 1;
}


int main(int ac, char **av)
{
	if (ac != 5)
	{
		printf("wrong number of arguments, usage : ...\n");
		return 0;
	}

	char *source_ip  = av[1]; // l'IP que tu vas usurper (IP_A)
	char *source_mac = av[2]; // la fausse MAC que tu vas annoncer (MAC_toi)
	char *target_ip  = av[3]; // la victime à empoisonner
	char *target_mac = av[4]; // la vraie MAC de la victime 



	// IPs : 4 octets
	uint8_t source_ip_converted[4];   // uint car un chiffre par octet. si avait fait tableau d int soit on aurait les 4 chiffre sur le meme octet (pas bon)  sooit 1 par 4 octet donc de la perte useless
	uint8_t target_ip_converted[4];  // uint permet d avoit 4 * 1 octet 
	
	// Pour les MACs : 6 octets
	uint8_t source_mac_converted[6];
	uint8_t target_mac_converted[6];

	
	if (!ip_getter(source_ip , source_ip_converted))
		return 1;
	if (!ip_getter(target_ip, target_ip_converted))
		return 1;
	if (!MAC_getter(source_mac , source_mac_converted))
		return 1;
	if (!MAC_getter(target_mac, target_mac_converted))
		return 1;
	


	int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));  // Af = Adresse familly (INET ipv4 = TCP/UDP etc.) ici AF_PACKET  =    layer 2 Ethernet/ARP ,              SOCK RAW = le kernell donne la trame brut.  (STREAM, tcp gere tout,  DGRAM  UDP le kernelle gere et envoi paquetes isoles ),       irc c etait 0 (tcp devine), ici c est ETH_P_ARP      filtre trames ARP, kernell check et evoit que les trames ARP                  
	if (sockfd < 0)
	{
		perror("socket");
		return 1;
	}
	// pour l isntant juste la structure, le socket ne sait pas OU ecouter

	// faut bind.





	struct ifaddrs *ifaddr; // besoin de  struct sockaddr_ll  pour 
	if (getifaddrs(&ifaddr) < 0)// demande au kernell tout les interface reseaux actives, remplit une liste chaine avec tout 
	{
		perror("getifaddrs");
		return 1;
	}

	struct ifaddrs *ptr = ifaddr;


	while (ptr)
	{
		if (ptr->ifa_flags & IFF_RUNNING && !(ptr->ifa_flags & IFF_LOOPBACK))  // verifie si interface active +  est ce que cest un lo (loopback)   si non  = eth0 (notre interface)
			break;
		ptr = ptr->ifa_next;
	}
	if (!ptr)
	{
		printf("no interface found\n");
		freeifaddrs(ifaddr);
		return 1;
	}

	int ifindex =  if_nametoindex(ptr->ifa_name); // prend l index numerique de l interface eth0 (2 par exemple)



	struct sockaddr_ll sll; // decrit adresse layer 2 : arp/ethernet
	sll.sll_family   = AF_PACKET; // uint16_t qui dit au kernetl cest une addresse 2 ethernet
	sll.sll_protocol = htons(ETH_P_ARP); //  from little to big endiant  pour stocker ETH p arp protocl
	sll.sll_ifindex  = ifindex; // index de notre interface
	if (bind(sockfd, (struct sockaddr *)&sll, sizeof(sll)) < 0) //  bind = syscall  qui dit lie le socket a cette interface on va ecouter tout dessus plus tard
	{
		perror("bind");
		return 1;
	}
	
	freeifaddrs(ifaddr); // free




	uint8_t buffer[42]; // taille exacte d'une trame ARP
	if (recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL) < 0) 	// syscall bloquant, process en sleeping dans kernel : maj renvoit copie des recu en buffer.       taille maxa lire, pas de flag,  pas de source pour savoir qui a send, pas  de taille de la source.
	{
		perror("recvfrom");
		return 1;
	}
	
	
	
	

	// PAS DE STRUCT pour recevoir les trames  comme on veut donc faut la rebuild non meme.
	
	
	
	struct ethernet_header *eth = (struct ethernet_header *)buffer;  // on prent les 14 premier octet du buffer et ils vont dans la structure (contigue d octet une structure en realite)
	struct arp_packet *arp = (struct arp_packet *)(buffer + 14); // les restat dans la partie tram arp
	
	

	//  reste a 

	// checker arp->opcode  == 1 (opcode veut dire : )  1 = request     2 arp reply   (3 et 4 obsoletes)
 	// dest ip ==  l ip qu on usurpe (source ip converted)
	// src ip = target == bien la personne que j arnaque

	return 0;
}