# TP_Linux


root@DE10-Standard:~# df -h
Filesystem      Size  Used Avail Use% Mounted on
/dev/root       3.0G  1.3G  1.5G  46% /
devtmpfs        375M     0  375M   0% /dev
tmpfs           376M     0  376M   0% /dev/shm
tmpfs           376M  9.8M  366M   3% /run
tmpfs           5.0M  4.0K  5.0M   1% /run/lock
tmpfs           376M     0  376M   0% /sys/fs/cgroup
tmpfs            76M     0   76M   0% /run/user/0

############################################################

root@DE10-Standard:~# df -h
Filesystem      Size  Used Avail Use% Mounted on
/dev/root        14G  1.3G   12G  10% /
devtmpfs        375M     0  375M   0% /dev
tmpfs           376M     0  376M   0% /dev/shm
tmpfs           376M  9.8M  366M   3% /run
tmpfs           5.0M  4.0K  5.0M   1% /run/lock
tmpfs           376M     0  376M   0% /sys/fs/cgroup
tmpfs            76M     0   76M   0% /run/user/0

############################################################

# Module final
Liens utiles:

https://man7.org/linux/man-pages/man2/init_module.2.html

https://www.kernel.org/doc/html/next/core-api/printk-basics.html

https://man.linuxreviews.org/man9/proc_dir_entry.9.html

https://www.unix.com/man_page/suse/9/filp_open/

## Choix de la vitesse de balayage par une option au moment du chargement du module
Pour définir la vitesse de balayage, il suffit de rajouter une variable

    module_param(speed, uint, 0644);
    MODULE_PARM_DESC(speed, "Vitesse du chenillard en millisecondes");

    // https://man7.org/linux/man-pages/man2/init_module.2.html

Il est possible d'utiliser ce paramètre dans notre code, par exemple:

    pr_info("Module chargé avec speed = %u ms\n", speed);
Pour initialiser le module avec notre paramètre il suffit de faire
    
    sudo insmod chenillard.ko speed=1000
## Récupération de la vitesse courante par lecture du fichier /proc/ensea/speed
Pour récupérer un paramètre dans un fichier, par exemple /proc/ensea/speed, il faut dans un premier temps créer un module qui crée le fichier /proc/ensea/speed
Pour cela, nous aurons besoin de la structure
  
    static struct proc_dir_entry *ensea_dir;    // Dossier /proc/ensea
    static struct proc_dir_entry *speed_file;   // Fichier /proc/ensea/speed
    
    // https://man.linuxreviews.org/man9/proc_dir_entry.9.html
  Puis dans l'initialisation de notre module, nous devrons rajouter
  
     ensea_dir = proc_mkdir("ensea", NULL);
  Cela nous permet d'avoir le dossier ensea dans /proc puis pour avoir le fichier /proc/ensea/speed nous devons utiliser cette fonction avec en second paramètre les droits pour notre fichier (0666 lecture/ecriture pour tout le monde)
  
    speed_file = proc_create(speed, 0666, ensea_dir, &speed_fops);
  Il ne faut pas oublier d'ajouter la structure qui indique au noyau quelles fonctions utiliser quand un utilisateur interagit avec un fichier du système et d'ajouter la fonction qui permet de lire notre fichier
  
    static const struct proc_ops speed_fops = {
        .proc_read = speed_read,
    };

    static ssize_t speed_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
        char buffer[32];
        int len;
      
        if (*ppos > 0)
            return 0;
        len = snprintf(buffer, sizeof(buffer), "%u\n", speed);
        if (copy_to_user(buf, buffer, len))
            return -EFAULT;
        *ppos = len;
        return len;
    }
  Pour finir, il ne faut pas oublier de nettoyer notre module. 
    
    static void __exit chenillard_exit(void) {
        remove_proc_entry(speed, ensea_dir);
        remove_proc_entry(ensea, NULL);
        pr_info("chenillard: module unloaded\n");
    }
## Modification de la patern par écriture dans le fichier /dev/ensea-led
## Récupération du patern courant par lecture du fichier /dev/ensea-led

Pour cela nous avons écrit 2 fonctions:

    static int read_led_state(void) {
        char buffer[2];
        int ret;
        loff_t pos = 0;  

        file = filp_open("/dev/ensea-led", O_RDONLY, 0);
        if (IS_ERR(file)) {
            pr_err("Failed to open /dev/ensea-led\n");
            return PTR_ERR(file);
        }
        ret = vfs_read(file, buffer, sizeof(buffer), &pos);
        if (ret < 0) {
            pr_err("Failed to read from /dev/ensea-led\n");
            filp_close(file, NULL);
            return ret;
        }
        pr_info("LED state: %c\n", buffer[0]); 
        filp_close(file, NULL);
        return 0;
    }

    static int write_led_state(char state) {
        char buffer[2];
        int ret;
        loff_t pos = 0;

        // Valider la valeur d'entrée (seulement '1' ou '0')
        if (state != '1' && state != '0') {
            pr_err("Invalid state. Use '1' or '0'.\n");
            return -EINVAL;
        }
        file = filp_open("/dev/ensea-led", O_WRONLY, 0);
        if (IS_ERR(file)) {
            pr_err("Failed to open /dev/ensea-led\n");
            return PTR_ERR(file);
        }
        buffer[0] = state;
        buffer[1] = '\0';
        ret = vfs_write(file, buffer, 1, &pos);
        if (ret < 0) {
            pr_err("Failed to write to /dev/ensea-led\n");
            filp_close(file, NULL);
            return ret;
        }
        filp_close(file, NULL);
        pr_info("LED state set to: %c\n", state);
        return 0;
    }
## Modification du sens de balayage par écriture du fichier /proc/ensea/dir
## Récupération du sens de balayage par lecture du fichier /proc/ensea/dir
Voire la partie "Choix de la vitesse de balayage par une option au moment du chargement du module"

## Thread Noyau
Pour réaliser notre chenillard, nous avons choisi d'utiliser un Thread.
Dans un premier temps, nous avons déclaré les variables globales de notre thread

    static struct task_struct *chenillard_thd;
    static int thread_running = 1;
Puis la fonction de notre thread :

    static int chenillard_thd_fonction(void *data)
    {
        while (!kthread_should_stop()) {
            if (thread_running) {
                pr_info("Thread noyau : Je suis un chenillard...\n");
                msleep(500);  // Attente de 500 ms
            } else {
                msleep(100);  // Repos si non actif
            }
        }
        pr_info("Thread noyau : je m'arrête.\n");
        return 0;
    }
il faut maintenant initialiser notre thread dans notre module:

    static int __init ensea_init(void)
    {
        pr_info("Module chargé : création du thread\n");

        chenillard_thd = kthread_create(chenillard_thd_fonction, NULL, "ensea_chenillard_thread");

        if (IS_ERR(chenillard_thd)) {
            pr_err("Échec de création du thread\n");
            return PTR_ERR(chenillard_thd);
        }

        wake_up_process(my_thread);  // Démarre réellement le thread
        return 0;
    }
Et pour finir, il ne faut pas oublier de kill notre thread à la sortie du module

    static void __exit ensea_exit(void)
    {  
        pr_info("Module déchargé : arrêt du thread\n");
        if (chenillard_thd)
            kthread_stop(chenillard_thd);
    }
