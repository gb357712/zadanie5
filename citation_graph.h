#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <exception>
#include <typeinfo>
#include <iostream>
#include <vector>
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <exception>
#include <typeinfo>
#include <iostream>
#include <vector>


//TODO sprawdzic noexcept

class PublicationAlreadyCreated: public std::exception
{
    const char* what() const noexcept override
    {
        return "PublicationAlreadyCreated";
    }
};

class PublicationNotFound: public std::exception
{
    const char* what() const noexcept override
    {
        return "PublicationNotFound";
    }
};

class TriedToRemoveRoot: public std::exception
{
    const char* what() const noexcept override
    {
        return "TriedToRemoveRoot";
    }
};


template <class Publication>
struct Node{
    std::vector< std::weak_ptr< Node<Publication> > > parents;
    std::set< std::shared_ptr< Node<Publication> > > children;
    std::unique_ptr<Publication> pub;
};

template <typename Publication>
class CitationGraph{
public:
    typedef typename Publication::id_type id_type_t;
    explicit CitationGraph(id_type_t const &stem_id);
    CitationGraph(CitationGraph<Publication> &&other) noexcept = default;
    CitationGraph<Publication>& operator=(CitationGraph<Publication> &&other) noexcept = default;
    const id_type_t get_root_id() const noexcept(noexcept(std::declval<Publication>().get_id()));
    std::vector<id_type_t> get_children(id_type_t const &id) const;
    std::vector<id_type_t> get_parents(id_type_t const &id) const;
    //a co z pamiecia na wskaznik?
    bool exists(id_type_t const &id) const noexcept;
    Publication& operator[](id_type_t const &id) const;
    void create(id_type_t const &id, id_type_t const &parent_id);
    void create(id_type_t const &id, std::vector<id_type_t> const &parent_ids);
    void add_citation(const id_type_t &child_id, const id_type_t &parent_id);
    void remove(id_type_t const &id);
private:
    //TODO wskaznik na mape?
    std::unordered_map<id_type_t, std::weak_ptr< Node<Publication> > > pubs;
    std::shared_ptr< Node<Publication> > root;
};


template<typename Publication>
CitationGraph<Publication>::CitationGraph(const id_type_t &stem_id){
    Node<Publication> tmp;
    tmp.pub = std::make_unique<Publication>(Publication(stem_id));
    root = std::make_shared< Node<Publication> >(std::move(tmp));
    pubs[root->pub->get_id()] = root;
}


template<typename Publication>
const typename Publication::id_type CitationGraph<Publication>::get_root_id() const noexcept(noexcept(std::declval<Publication>().get_id())) {
    return root->pub->get_id();
}

template<typename Publication>
std::vector<typename Publication::id_type> CitationGraph<Publication>::get_children(const id_type_t &id) const {
    if(!exists(id))
        throw PublicationNotFound();
    std::vector<id_type_t> res {};
    //TODO asercje na poprawnosc?
    std::shared_ptr< Node<Publication> > spt = pubs.at(id).lock();
    for(std::shared_ptr< Node<Publication> > child : spt->children)
      res.emplace_back(child->pub->get_id());
    return res;
}

template<typename Publication>
std::vector<typename Publication::id_type> CitationGraph<Publication>::get_parents(const id_type_t &id) const {
    if(!exists(id))
        throw PublicationNotFound();
    std::vector<id_type_t> res;
    //TODO asercje na poprawnosc?
    std::shared_ptr< Node<Publication> > spt = pubs.at(id).lock();
    for(std::weak_ptr< Node<Publication> > parent : spt->parents)
    {
        std::shared_ptr< Node<Publication> > sptParent = parent.lock();
        res.emplace_back(sptParent->pub->get_id());
    }
    return res;
}

template<typename Publication>
bool CitationGraph<Publication>::exists(id_type_t const &id) const noexcept{
    return pubs.find(id) != pubs.end() && pubs.at(id).use_count()!=0;
}

template<typename Publication>
Publication &CitationGraph<Publication>::operator[](id_type_t const &id) const {
    if(!exists(id))
        throw PublicationNotFound();
    std::shared_ptr< Node<Publication> > tmp = pubs.at(id).lock();
    return *(tmp->pub);
}

template<typename Publication>
void CitationGraph<Publication>::create(const id_type_t &id, const id_type_t &parent_id) {
    if(exists(id))
        throw PublicationAlreadyCreated();
    if(!exists(parent_id))
        throw PublicationNotFound();
    Node<Publication> tmp;
    tmp.pub = std::make_unique<Publication>(Publication(id));
    std::weak_ptr< Node<Publication> > parent = pubs.at(parent_id);
    tmp.parents.emplace_back(parent);
    std::shared_ptr< Node<Publication> > tmpPtr = std::make_shared< Node<Publication> >(std::move(tmp));
    pubs[id] = tmpPtr;
    std::shared_ptr< Node<Publication> > sptParent = parent.lock();
    sptParent->children.insert(tmpPtr);
}

template<typename Publication>
void CitationGraph<Publication>::create(const id_type_t &id, const std::vector<id_type_t> &parent_ids) {
    if(exists(id))
        throw PublicationAlreadyCreated();
    for(auto parent_id:parent_ids)
        if(!exists(parent_id))
            throw PublicationNotFound();
    Node<Publication> tmp;
    tmp.pub = std::make_unique<Publication>(Publication(id));
    std::shared_ptr< Node<Publication> > tmpPtr = std::make_shared< Node<Publication> >(std::move(tmp));
    pubs[id] = tmpPtr;
    for(id_type_t parent_id : parent_ids)
    {
        std::weak_ptr< Node<Publication> > parent = pubs[parent_id];
        tmpPtr->parents.emplace_back(parent);
        std::shared_ptr< Node<Publication> > sptParent = parent.lock();
        sptParent->children.insert(tmpPtr);
    }

}

template<typename Publication>
void CitationGraph<Publication>::add_citation(const id_type_t &child_id, const id_type_t &parent_id) {
    if(!exists(child_id) || !exists(parent_id))
        throw PublicationNotFound();
    std::shared_ptr< Node<Publication> > childPtr = pubs[child_id].lock();
    for(std::weak_ptr< Node<Publication> > parent : childPtr->parents) //sprawdzam czy juz krawedz nie istnieje bo w secie nie będzie duplikatow ale w vectorze juz tak a chyba nie da sie miec seta<weak_ptr>
    {
      std::shared_ptr< Node<Publication> > sptParent = parent.lock();
      if(sptParent->pub->get_id()==parent_id)
        return;
    }
    std::shared_ptr< Node<Publication> > parentPtr = pubs[parent_id].lock();

    parentPtr->children.insert(childPtr);
    childPtr->parents.emplace_back(parentPtr);

}

template<typename Publication>
void CitationGraph<Publication>::remove(const id_type_t &id) {
    if(!exists(id))
        throw PublicationNotFound();
    if(id == root->pub->get_id())
        throw TriedToRemoveRoot();
//    std::cerr<< "1 " << pubs.size()<<"\n";
//    std::cerr<< "2 "<<pubs[id].use_count()<<"\n";
    std::shared_ptr< Node<Publication> > ptr = pubs[id].lock();
    for(std::weak_ptr<Node<Publication>> parent : ptr->parents)
    {
        std::shared_ptr< Node<Publication> > parPtr = parent.lock();
        parPtr->children.erase(ptr);
    }
    ptr.reset();
    pubs.erase(id);
//    std::cerr<< "3 "<<pubs.size()<<"\n";
//    std::cerr<< "4 "<<pubs[id].use_count()<<"\n";

}



#ifndef ZAD5_CITATION_GRAPH_H
#define ZAD5_CITATION_GRAPH_H

#endif //ZAD5_CITATION_GRAPH_H
